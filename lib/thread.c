#include <mmu.h>
#include <error.h>
#include <env.h>
#include <kerelf.h>
#include <sched.h>
#include <pmap.h>
#include <printf.h>

struct Thread *curthread = NULL;
struct Thread_list thread_sched_list[2];
extern char *KERNEL_SP;

static u_int thread_index_alloc(struct Env *e) {
    u_int *bitmap = &e->env_thread_bitmap;
    int i;
    for (i = 0; i < 32; ++i) {
        if ((*bitmap & (1 << i)) == 0) {
            *bitmap |= (1 << i);
            return i;
        }
    }
    panic("too many threads!");
}

static void thread_index_free(struct Env *e, u_int index) {
    if (index > 31) panic("index of thread must be less than 32!"); 
    e->env_thread_bitmap &= ~(1 << index);
}

static u_int mkthreadid(struct Env* e) {
    int index = thread_index_alloc(e);
//	printf("index is %d env_id is %d", index, e->env_id);
    return (e->env_id << 5) | index;
}


int id2thread(u_int thread_id, struct Thread **pthread) {
    struct Thread *t;
    struct Env *e;
    // return current thread when `thread_id` is 0
    if (thread_id == 0) {
        *pthread = curthread;
        return 0;
    }
    // get env and thread
    e = envs + ENVX(THREAD2ENVID(thread_id));
    t = &e->env_threads[THREAD2INDEX(thread_id)];

    if (t->thread_id != thread_id) {
        *pthread = NULL;
        return -E_BAD_THREAD;
    }
    *pthread = t;
    return 0;
}

int thread_alloc(struct Env *e, struct Thread **new) {
    int ret;
    struct Thread *t;
    u_int thread_id;
    
    thread_id = mkthreadid(e);
    t = &e->env_threads[THREAD2INDEX(thread_id)];
    printf("\033[1;33;40m>>> thread %d is alloced ... (threads[%d] of env %d) <<<\033[0m\n", thread_id, THREAD2INDEX(thread_id), THREAD2ENVID(thread_id));
    // color_printf(1, 33, ">>> thread %d is alloced <<<", thread_id);

    // initiate the tcb
    t->thread_id = thread_id;
    t->thread_pri = e->env_pri;
    t->thread_tag = 0;
    t->thread_status = THREAD_RUNNABLE;
    t->thread_retval = 0;
    t->thread_retval_ptr = 0;
    t->thread_join_caller = 0;
    t->thread_cancel_type = 0;

    t->thread_tf.cp0_status = 0x1000100c;
    t->thread_tf.regs[29] = USTACKTOP - 1024 * BY2PG * THREAD2INDEX(thread_id);

    *new = t;
    return 0;
}


void thread_free(struct Thread *t) {
    struct Env *e;
    e = envs + ENVX(THREAD2ENVID(t->thread_id));
    thread_index_free(e, THREAD2INDEX(t->thread_id));
    t->thread_status = THREAD_FREE;
    LIST_REMOVE(t, thread_sched_link);
}


void thread_destroy(struct Thread *t) {
    struct Env *e;
    e = envs + ENVX(THREAD2ENVID(t->thread_id));

    thread_free(t);

    if (curthread == t) {
        curthread = NULL;
    }
    
    bcopy((void *)KERNEL_SP - sizeof(struct Trapframe),
            (void *)TIMESTACK - sizeof(struct Trapframe),
            sizeof(struct Trapframe));
    printf("\033[1;35;40m>>> thread %d is killed ... (threads[%d] of env %d) <<<\033[0m\n", t->thread_id, THREAD2INDEX(t->thread_id), THREAD2ENVID(t->thread_id));
    // color_printf(1, 35, ">>> thread %d is killed ... <<<", t->thread_id);
    if (e->env_thread_bitmap == 0) {
        env_free(e);
        printf("\033[1;35;40m>>> env %d is killed ...  <<<\033[0m\n", e->env_id);
        // color_printf(1, 35, ">>> env %d is killed ... <<<", t->thread_id);
    }
    sched_yield();
}

extern void env_pop_tf(struct Trapframe *tf, int id);
extern void lcontext(u_int contxt);

void thread_run(struct Thread *t) {
    struct Env *e;
    e = envs + ENVX(THREAD2ENVID(t->thread_id));

    if (curthread != NULL) {
        struct Trapframe *old;
        old = (struct Trapframe *)(TIMESTACK - sizeof(struct Trapframe));
        bcopy(old, &(curthread->thread_tf), sizeof(struct Trapframe));
        curthread->thread_tf.pc = old->cp0_epc;
    }

    if (curenv != e) {
        curenv = e;
        lcontext(curenv->env_pgdir);
    }

    curthread = t;
    env_pop_tf(&t->thread_tf, GET_ENV_ASID(e->env_id));
}
