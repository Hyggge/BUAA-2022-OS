#include "lib.h"
#include <unistd.h>
#include <mmu.h>
#include <env.h>
#include <trap.h>

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void * (*start_rountine)(void *), void *arg) {
    u_int thread_id;
    struct Thread *t;
    
    thread_id = syscall_thread_alloc();
    if (thread_id < 0) {
        *thread = NULL;
        return thread_id;
    }

    t = &env->env_threads[THREAD2INDEX(thread_id)];
    t->thread_tf.pc = start_rountine;
    t->thread_tf.regs[4] = arg;
    t->thread_tf.regs[29] -= 4;
    t->thread_tf.regs[31] = exit;

    syscall_set_thread_status(thread_id, THREAD_RUNNABLE);
    *thread = thread_id;
    return 0;
}

void pthread_exit(void *retval) {
    u_int thread_id;
    struct Thread *cur;

    thread_id = syscall_get_thread_id();
    cur = &env->env_threads[THREAD2INDEX(thread_id)];
    cur->thread_retval = retval;
   	cur->thread_tag |= THREAD_TAG_EXITED;
    exit();
}

int pthread_cancel(pthread_t thread) {
    struct Thread *t;
    t = &env->env_threads[THREAD2INDEX(thread)];

    if (t->thread_id != thread || t->thread_status == THREAD_FREE) {
        return -E_THREAD_NOT_FOUND;
    } 
    else if ((t->thread_tag & THREAD_TAG_DETACHED) != 0) {
        return -E_THREAD_DETACHED;
    }
    else if ((t->thread_tag & THREAD_TAG_CANCELED) != 0) {
        return -E_THREAD_CANCELED;
    }
	else if ((t->thread_tag & THREAD_TAG_EXITED) != 0) {
        return -E_THREAD_EXITED;
    }
    else {
        t->thread_tag |= THREAD_TAG_CANCELED;
    	t->thread_retval = PTHREAD_CANCELED;
        if (t->thread_cancel_type == THREAD_CANCEL_ASYNCHRONOUS) {
			if (thread == syscall_get_thread_id()) exit();
		    else t->thread_tf.pc = exit;
        }
	}
    return 0;
}

int pthread_join(pthread_t thread, void **retval_ptr) {
    return syscall_thread_join(thread, retval_ptr);
}


int pthread_detach(pthread_t thread) {
    struct Thread *dst;
    dst = &env->env_threads[THREAD2INDEX(thread)];

    if (dst->thread_id != thread  || dst->thread_status == THREAD_FREE) {
        return -E_THREAD_NOT_FOUND;
    } 
    else if ((dst->thread_tag & THREAD_TAG_DETACHED) != 0) {
        return -E_THREAD_DETACHED;
    }
    else if ((dst->thread_tag & THREAD_TAG_JOINED) != 0) {
        return -E_THREAD_JOINED;
    }

    dst->thread_tag |= THREAD_TAG_DETACHED;
    return 0;
}


int pthread_setcanceltype(int type, int *oldtype) {
    u_int thread_id;
    struct Thread *cur;

    thread_id = syscall_get_thread_id();
    cur = &env->env_threads[THREAD2INDEX(thread_id)];

    if (oldtype) {
        *oldtype = cur->thread_cancel_type;
    }

    cur->thread_cancel_type = type;
    return 0;
}

void pthread_testcancel(void) {
    u_int thread_id;
    struct Thread *cur;

    thread_id = syscall_get_thread_id();
    cur = &env->env_threads[THREAD2INDEX(thread_id)];
    if ((cur->thread_tag & THREAD_TAG_CANCELED) != 0 &&
            cur->thread_cancel_type == THREAD_CANCEL_DEFERRED) {
        exit();
    }
}

pthread_t pthread_self() {
    return syscall_get_thread_id();
}





int sem_init (sem_t *sem, int pshared, unsigned int value) {
    return syscall_sem_init(sem, pshared, value);
}

int sem_destroy (sem_t *sem) {
    return syscall_sem_destroy(sem);
}

int sem_wait (sem_t *sem) {
    return syscall_sem_wait(sem);
}

int sem_trywait(sem_t *sem) {
    return syscall_sem_trywait(sem);
}

int sem_post (sem_t *sem) {
    return syscall_sem_post(sem);
}

int sem_getvalue (sem_t *sem, int *valp) {
    return syscall_sem_getvalue(sem, valp);
}
