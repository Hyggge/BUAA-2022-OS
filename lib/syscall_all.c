#include "../drivers/gxconsole/dev_cons.h"
#include <mmu.h>
#include <env.h>
#include <printf.h>
#include <pmap.h>
#include <sched.h>

extern char *KERNEL_SP;
extern struct Env *curenv;

/* Overview:
 * 	This function is used to print a character on screen.
 *
 * Pre-Condition:
 * 	`c` is the character you want to print.
 */
void sys_putchar(int sysno, int c, int a2, int a3, int a4, int a5)
{
	printcharc((char) c);
	return ;
}


extern void lp_Print(void (*output)(void *, char *, int), void * arg, char *fmt, va_list ap);
extern void myoutput(void *arg, char *s, int l);
void sys_printf(int sysno, char *fmt, va_list* ap_ptr) 
{
	lp_Print(myoutput, 0, fmt, *ap_ptr);
}

/* Overview:
 * 	This function enables you to copy content of `srcaddr` to `destaddr`.
 *
 * Pre-Condition:
 * 	`destaddr` and `srcaddr` can't be NULL. Also, the `srcaddr` area
 * 	shouldn't overlap the `destaddr`, otherwise the behavior of this
 * 	function is undefined.
 *
 * Post-Condition:
 * 	the content of `destaddr` area(from `destaddr` to `destaddr`+`len`) will
 * be same as that of `srcaddr` area.
 */
void *memcpy(void *destaddr, void const *srcaddr, u_int len)
{
	char *dest = destaddr;
	char const *src = srcaddr;

	while (len-- > 0) {
		*dest++ = *src++;
	}

	return destaddr;
}

/* Overview:
 *	This function provides the environment id of current process.
 *
 * Post-Condition:
 * 	return the current environment id
 */
u_int sys_getenvid(void)
{
	return curenv->env_id;
}

/* Overview:
 *	This function enables the current process to give up CPU.
 *
 * Post-Condition:
 * 	Deschedule current process. This function will never return.
 *
 * Note:
 *  For convenience, you can just give up the current time slice.
 */
/*** exercise 4.6 ***/
void sys_yield(void)
{
	//curenv->env_status = ENV_NOT_RUNNABLE;
	bcopy((void *)KERNEL_SP - sizeof(struct Trapframe), 
		  	(void *)TIMESTACK - sizeof(struct Trapframe), 
			sizeof(struct Trapframe));
	sched_yield();
}

/* Overview:
 * 	This function is used to destroy the current environment.
 *
 * Pre-Condition:
 * 	The parameter `envid` must be the environment id of a
 * process, which is either a child of the caller of this function
 * or the caller itself.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 when error occurs.
 */
int sys_env_destroy(int sysno, u_int envid)
{
	/*
		printf("[%08x] exiting gracefully\n", curenv->env_id);
		env_destroy(curenv);
	*/
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0) {
		return r;
	}

	printf("[env %d] is destroying env %d\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

/* Overview:
 * 	Set envid's pagefault handler entry point and exception stack.
 *
 * Pre-Condition:
 * 	xstacktop points one byte past exception stack.
 *
 * Post-Condition:
 * 	The envid's pagefault handler will be set to `func` and its
 * 	exception stack will be set to `xstacktop`.
 * 	Returns 0 on success, < 0 on error.
 */
/*** exercise 4.12 ***/
int sys_set_pgfault_handler(int sysno, u_int envid, u_int func, u_int xstacktop)
{
	// Your code here.
	struct Env *env;
	int ret;

	ret = envid2env(envid, &env, 1);
	if (ret < 0) return ret;

	env->env_xstacktop = xstacktop;
	env->env_pgfault_handler = func; 

	return 0;
	//	panic("sys_set_pgfault_handler not implemented");
}

/* Overview:
 * 	Allocate a page of memory and map it at 'va' with permission
 * 'perm' in the address space of 'envid'.
 *
 * 	If a page is already mapped at 'va', that page is unmapped as a
 * side-effect.
 *
 * Pre-Condition:
 * perm -- PTE_V is required,
 *         PTE_COW is not allowed(return -E_INVAL),
 *         other bits are optional.
 *
 * Post-Condition:
 * Return 0 on success, < 0 on error
 *	- va must be < UTOP
 *	- env may modify its own address space or the address space of its children
 */
/*** exercise 4.3 ***/
int sys_mem_alloc(int sysno, u_int envid, u_int va, u_int perm)
{
	// Your code here.
	struct Env *env;
	struct Page *ppage;
	int ret;

	if (va >= UTOP) {
		return -E_INVAL;
	}

	if (perm & PTE_COW) {
		return -E_INVAL;
	}

	if ((perm & PTE_V) == 0) {
		return -E_INVAL;
	}
	
	ret = envid2env(envid, &env, 0);
	if (ret < 0) {
		return ret;
	}
	
	ret = page_alloc(&ppage);
	if (ret < 0) {
		return ret;
	}

	ret = page_insert(env->env_pgdir, ppage, va, perm);
	if (ret < 0) {
		return ret;
	}
	
	
	return 0;
}

/* Overview:
 * 	Map the page of memory at 'srcva' in srcid's address space
 * at 'dstva' in dstid's address space with permission 'perm'.
 * Perm must have PTE_V to be valid.
 * (Probably we should add a restriction that you can't go from
 * non-writable to writable?)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Note:
 * 	Cannot access pages above UTOP.
 */
/*** exercise 4.4 ***/
int sys_mem_map(int sysno, u_int srcid, u_int srcva, u_int dstid, u_int dstva,
				u_int perm)
{
	int ret;
	u_int round_srcva, round_dstva;
	struct Env *srcenv;
	struct Env *dstenv;
	struct Page *ppage;
	Pte *ppte;

	ppage = NULL;
	ret = 0;
	round_srcva = ROUNDDOWN(srcva, BY2PG);
	round_dstva = ROUNDDOWN(dstva, BY2PG);

    //your code here
	if (srcva >= UTOP || dstva >= UTOP) {
		return -E_INVAL;
	}

	if ((perm & PTE_V) == 0) {
		return -E_INVAL;
	}

	ret = envid2env(srcid, &srcenv, 0);
	if (ret < 0) {
		return ret;
	}
	
	ret = envid2env(dstid, &dstenv, 0);
	if (ret < 0) {
		return ret;
	}

	ppage = page_lookup(srcenv->env_pgdir, round_srcva, &ppte);
	if (ppte == NULL) {
		return -E_INVAL;
	}
	if ((!(*ppte & PTE_R)) && (perm & PTE_R)) {
		return -E_INVAL;
	}
	ret = page_insert(dstenv->env_pgdir, ppage, round_dstva, perm);
	
	return ret;
}

/* Overview:
 * 	Unmap the page of memory at 'va' in the address space of 'envid'
 * (if no page is mapped, the function silently succeeds)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Cannot unmap pages above UTOP.
 */
/*** exercise 4.5 ***/
int sys_mem_unmap(int sysno, u_int envid, u_int va)
{
	// Your code here.
	int ret;
	struct Env *env;

	if (va >= UTOP) {
		return -E_INVAL;
	}

	ret = envid2env(envid, &env, 0);
	if (ret < 0) {
		return ret;
	}

	page_remove(env->env_pgdir, va);
	
	return 0;
	//	panic("sys_mem_unmap not implemented");
}

/* Overview:
 * 	Allocate a new environment.
 *
 * Pre-Condition:A
 * The new child is left as env_alloc created it, except that
 * status is set to ENV_NOT_RUNNABLE and the register set is copied
 * from the current environment.
 *
 * Post-Condition:
 * 	In the child, the register set is tweaked so sys_env_alloc returns 0.
 * 	Returns envid of new environment, or < 0 on error.
 */
/*** exercise 4.8 ***/
int sys_env_alloc(void)
{
	// Your code here.
	int ret;
	struct Env *e;
	struct Thread *t;

	ret = env_alloc(&e, curenv->env_id);
	if (ret < 0) return ret;

	e->env_pri = curenv->env_pri;

	ret = thread_alloc(e, &t);
	if (ret < 0) return ret;

	bcopy((void *)KERNEL_SP - sizeof(struct Trapframe), (void *)&(e->env_threads[0].thread_tf), sizeof(struct Trapframe));
	e->env_threads[0].thread_tf.pc = e->env_threads[0].thread_tf.cp0_epc;
	e->env_threads[0].thread_tf.regs[2] = 0;
	e->env_threads[0].thread_status = THREAD_NOT_RUNNABLE;

	return e->env_id;
	//	panic("sys_env_alloc not implemented");
}

/* Overview:
 * 	Set envid's env_status to status.
 *
 * Pre-Condition:
 * 	status should be one of `ENV_RUNNABLE`, `ENV_NOT_RUNNABLE` and
 * `ENV_FREE`. Otherwise return -E_INVAL.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if status is not a valid status for an environment.
 * 	The status of environment will be set to `status` on success.
 */
/*** exercise 4.14 ***/
int sys_set_env_status(int sysno, u_int envid, u_int status)
{
	// // Your code here.
	// struct Env *env;
	// int ret;
	
	// if (status != ENV_RUNNABLE && status != ENV_FREE && status != ENV_NOT_RUNNABLE) {
	// 	return -E_INVAL;
	// }

	// ret = envid2env(envid, &env, 0);
	// if (ret < 0) return ret;


	// if (status == ENV_RUNNABLE && env->env_status != ENV_RUNNABLE) {
	// 	LIST_INSERT_HEAD(&env_sched_list[0], env, env_sched_link);
	// }
	// if (status != ENV_RUNNABLE && env->env_status == ENV_RUNNABLE) {
	// 	LIST_REMOVE(env, env_sched_link);
	// }

	// // env->env_status = status;
	return 0;
	// //	panic("sys_env_set_status not implemented");
}

/* Overview:
 * 	Set envid's trap frame to tf.
 *
 * Pre-Condition:
 * 	`tf` should be valid.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if the environment cannot be manipulated.
 *
 * Note: This hasn't be used now?
 */
int sys_set_trapframe(int sysno, u_int envid, struct Trapframe *tf)
{

	return 0;
}

/* Overview:
 * 	Kernel panic with message `msg`.
 *
 * Pre-Condition:
 * 	msg can't be NULL
 *
 * Post-Condition:
 * 	This function will make the whole system stop.
 */
void sys_panic(int sysno, char *msg)
{
	// no page_fault_mode -- we are trying to panic!
	panic("%s", TRUP(msg));
}

/* Overview:
 * 	This function enables caller to receive message from
 * other process. To be more specific, it will flag
 * the current process so that other process could send
 * message to it.
 *
 * Pre-Condition:
 * 	`dstva` is valid (Note: NULL is also a valid value for `dstva`).
 *
 * Post-Condition:
 * 	This syscall will set the current process's status to
 * ENV_NOT_RUNNABLE, giving up cpu.
 */
/*** exercise 4.7 ***/
void sys_ipc_recv(int sysno, u_int dstva)
{
	if (dstva >= UTOP) return;

	curenv->env_ipc_recving = 1;
	curenv->env_ipc_dstva = dstva;
	curenv->env_ipc_dst_thread = curthread->thread_id;
	curthread->thread_status = THREAD_NOT_RUNNABLE;
	sys_yield();
}

/* Overview:
 * 	Try to send 'value' to the target env 'envid'.
 *
 * 	The send fails with a return value of -E_IPC_NOT_RECV if the
 * target has not requested IPC with sys_ipc_recv.
 * 	Otherwise, the send succeeds, and the target's ipc fields are
 * updated as follows:
 *    env_ipc_recving is set to 0 to block future sends
 *    env_ipc_from is set to the sending envid
 *    env_ipc_value is set to the 'value' parameter
 * 	The target environment is marked runnable again.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Hint: the only function you need to call is envid2env.
 */
/*** exercise 4.7 ***/
int sys_ipc_can_send(int sysno, u_int envid, u_int value, u_int srcva,
					 u_int perm)
{

	int ret;
	struct Env *e;
	struct Page *p;
	struct Thread *t;

	if (srcva >= UTOP) return -E_INVAL;

	ret = envid2env(envid, &e, 0);
	if (ret < 0) return ret;
	
	if (e->env_ipc_recving == 0) return -E_IPC_NOT_RECV;

	e->env_ipc_recving = 0;
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_value = value;
	e->env_ipc_perm = perm;

	t = &e->env_threads[THREAD2INDEX(e->env_ipc_dst_thread)];
	t->thread_status = THREAD_RUNNABLE;

	u_int dstva = e->env_ipc_dstva;

	if (srcva != 0) {
		p = page_lookup(curenv->env_pgdir, srcva, NULL);
		if (p == NULL) return -E_INVAL;
		page_insert(e->env_pgdir, p, dstva, perm);
	}

	return 0;
}

// pthread
int sys_thread_alloc(int sysno) {
	int ret;
	struct Thread *t;

	ret = thread_alloc(curenv, &t);
	if (ret < 0) return ret;

	t->thread_tf.pc = t->thread_tf.cp0_epc;	
	t->thread_tf.regs[2] = 0;
	t->thread_status = THREAD_NOT_RUNNABLE;

	return t->thread_id;
}


int sys_thread_destroy(int sysno, u_int threadid) {
	int ret;
	struct Thread *t;

	ret = id2thread(threadid, &t);
	if (ret < 0) return ret;

	if (t->thread_status == THREAD_FREE) {
		return -E_INVAL;
	}

	if ((t->thread_tag & THREAD_TAG_JOINED) != 0) {
		u_int caller_id = t->thread_join_caller;
		struct Thread * caller = &curenv->env_threads[THREAD2INDEX(caller_id)];
		t->thread_tag &= ~THREAD_TAG_JOINED;
		if (caller->thread_retval_ptr != NULL) {
			*(caller->thread_retval_ptr) = t->thread_retval;
			//printf("caller is %d, retval_ptr is %d", caller_id, t->thread_retval);
		}
		caller->thread_status = THREAD_RUNNABLE;
		// LIST_INSERT_HEAD(&thread_sched_list[0], caller, thread_sched_link);
	}  

//	printf("[thread %d] destroying thread %d\n", curthread->thread_id, t->thread_id);
	thread_destroy(t);
	return 0;
}

int sys_set_thread_status(int sysno, u_int threadid, u_int status) {
	int ret;
	struct Thread *t;

	if (status != THREAD_RUNNABLE && status != THREAD_FREE && status != THREAD_NOT_RUNNABLE) {
		return -E_INVAL;
	}

	ret = id2thread(threadid, &t);
	if (ret < 0) return ret;

	if (status == THREAD_RUNNABLE && t->thread_status != THREAD_RUNNABLE) {
		LIST_INSERT_HEAD(&thread_sched_list[0], t, thread_sched_link);
	}

	if (status != THREAD_RUNNABLE && t->thread_status == THREAD_RUNNABLE) {
		LIST_REMOVE(t, thread_sched_link);
	}

	t->thread_status = status;
	return 0;
}

int sys_get_thread_id(int sysno) {
	return curthread->thread_id;
}

int sys_thread_join(int sysno, u_int thread_id, void **retval_ptr) {
	struct Thread *dst;
	int ret = id2thread(thread_id, &dst);
	if (ret < 0) return ret; 

	if (dst->thread_id != thread_id) {
        return -E_THREAD_NOT_FOUND;
    } 
    else if ((dst->thread_tag & THREAD_TAG_DETACHED) != 0) {
        return -E_THREAD_DETACHED;
    }
    else if ((dst->thread_tag & THREAD_TAG_JOINED) != 0) {
        return -E_THREAD_JOINED;
    }

	if (dst->thread_status == THREAD_FREE) {
//		printf("%d enter pthread_join, dst is %d, dst''s status is %d\n", curthread->thread_id, dst->thread_id, dst->thread_status);
        if (retval_ptr != NULL)
			*retval_ptr = dst->thread_retval;
       	return 0;
    }

 	dst->thread_tag |= THREAD_TAG_JOINED;
    dst->thread_join_caller = curthread->thread_id;
// syscall_set_thread_status(0, THREAD_NOT_RUNNABLE);
    curthread->thread_retval_ptr = retval_ptr;
    curthread->thread_status = THREAD_NOT_RUNNABLE;

	struct Trapframe *tf = (struct Trapframe *)(KERNEL_SP - sizeof(struct Trapframe));
	tf->regs[2] = 0;
    sys_yield();
}













// semaphore 
int sys_sem_init (int sysno, sem_t *sem, int pshared, unsigned int value) {
	int i;

    if (sem == NULL) {
		return -E_SEM_NOT_FOUND;
	}

	sem->sem_value = value;
	sem->sem_queue_head = 0; 
	sem->sem_queue_tail = 0;
	sem->sem_perm |= SEM_PERM_VALID;

	if (pshared) {
		sem->sem_perm |= SEM_PERM_SHARE;
	}

	for (i = 0; i < 32; i++) {
		sem->sem_wait_queue[i] = NULL;
	}
	return 0;
}

int sys_sem_destroy (int sysno, sem_t *sem) {
	if ((sem->sem_perm & SEM_PERM_VALID) == 0) {
		return -E_SEM_INVALID;
	}
	if (sem->sem_queue_head != sem->sem_queue_tail) {
		return -E_SEM_DESTROY_FAIL;
	}
	sem->sem_perm &= ~SEM_PERM_VALID;
	return 0;
}

int sys_sem_wait (int sysno, sem_t *sem) {
	if ((sem->sem_perm & SEM_PERM_VALID) == 0) {
		return -E_SEM_INVALID;
	}
	
	sem->sem_value--;
	if (sem->sem_value >= 0) {
		return 0;
	}

	// if sem_value < 0 
	if (sem->sem_value < -32) {
		return -E_SEM_WAIT_MAX;
	}

	// must wait
	sem->sem_wait_queue[sem->sem_queue_tail] = curthread;
	sem->sem_queue_tail = (sem->sem_queue_tail + 1)	% 32;

	curthread->thread_status = THREAD_NOT_RUNNABLE;
	struct Trapframe *tf = (struct Trapframe *)(KERNEL_SP - sizeof(struct Trapframe));
	tf->regs[2] = 0;
	sys_yield();
}

int sys_sem_trywait(int sysno, sem_t *sem) {
	if ((sem->sem_perm & SEM_PERM_VALID) == 0) {
		return -E_SEM_INVALID;
	}
	
	sem->sem_value--;
	if (sem->sem_value >= 0) {
		return 0;
	}
	return -E_SEM_TRYWAIT_FAIL;
}

int sys_sem_post (int sysno, sem_t *sem) {
	struct Thread *t;

	if ((sem->sem_perm & SEM_PERM_VALID) == 0) {
		return -E_SEM_INVALID;
	}
	
	sem->sem_value++; 
	if (sem->sem_value <= 0) {
		t = sem->sem_wait_queue[sem->sem_queue_head];
		sem->sem_queue_head = (sem->sem_queue_head + 1) % 32;
		t->thread_status = THREAD_RUNNABLE;
	}
	return 0;
}

int sys_sem_getvalue (int sysno, sem_t *sem, int *valp) {
	if ((sem->sem_perm & SEM_PERM_VALID) == 0) {
		return -E_SEM_INVALID;
	}
	if (valp) {
		*valp = sem->sem_value;
	}
	return 0;
}
