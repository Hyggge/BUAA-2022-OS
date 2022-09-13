/* See COPYRIGHT for copyright information. */

#ifndef _ENV_H_
#define _ENV_H_

#include "types.h"
#include "queue.h"
#include "trap.h"
#include "mmu.h" 

#define LOG2NENV	10
#define NENV		(1<<LOG2NENV)
#define ENVX(envid)	((envid) & (NENV - 1))
#define GET_ENV_ASID(envid) (((envid)>> 11)<<6)

// Values of env_status in struct Env
#define ENV_FREE	0
#define ENV_RUNNABLE		1
#define ENV_NOT_RUNNABLE	2

// run status
#define THREAD_FREE	            0
#define THREAD_RUNNABLE		    1
#define THREAD_NOT_RUNNABLE	    2

// tags of thread
#define THREAD_TAG_CANCELED    1
#define THREAD_TAG_JOINED      2
#define THREAD_TAG_EXITED      4
#define THREAD_TAG_DETACHED    8

// cancel type of  thread
#define THREAD_CANCEL_DEFERRED  0
#define THREAD_CANCEL_ASYNCHRONOUS	1

// perms of semaphore
#define SEM_PERM_VALID          1
#define SEM_PERM_SHARE          2

// 
#define PTHREAD_CANCELED        666

#define THREAD2INDEX(x)         ((x) & 31)
#define THREAD2ENVID(x)         ((x) >> 5) 

struct Thread {
    u_int thread_id;
    u_int thread_pri;
    u_int thread_tag;
    u_int thread_status;
    struct Trapframe thread_tf;
    LIST_ENTRY(Thread) thread_sched_link;

    void* thread_retval;
    void** thread_retval_ptr;

    u_int thread_join_caller;

	u_int thread_cancel_type;
};

struct Semaphore {
    u_int sem_perm;
    int sem_value;
    u_int sem_queue_head;
    u_int sem_queue_tail;
    struct Thread* sem_wait_queue[32];
};

struct Env {
	// struct Trapframe env_tf;        // Saved registers
	LIST_ENTRY(Env) env_link;       // Free list
	u_int env_id;                   // Unique environment identifier
	u_int env_parent_id;            // env_id of this env's parent
	// u_int env_status;               // Status of the environment
	Pde  *env_pgdir;                // Kernel virtual address of page dir
	u_int env_cr3;
    u_int env_pri;
	
	LIST_ENTRY(Env) env_sched_link;

	// Lab 4 IPC
	u_int env_ipc_value;            // data value sent to us 
	u_int env_ipc_from;             // envid of the sender  
	u_int env_ipc_recving;          // env is blocked receiving
	u_int env_ipc_dstva;		// va at which to map received page
	u_int env_ipc_perm;		// perm of page mapping received

	// Lab 4 fault handling
	u_int env_pgfault_handler;      // page fault state
	u_int env_xstacktop;            // top of exception stack

	// Lab 6 scheduler counts
	u_int env_runs;			// number of times been env_run'ed
	u_int env_nop;                  // align to avoid mul instruction

	// Lab 4 challenge
	u_int env_thread_bitmap;
	struct Thread env_threads[31];
	u_int env_ipc_dst_thread;

};

LIST_HEAD(Env_list, Env);
extern struct Env *envs;		// All environments
extern struct Env *curenv;	        // the current env
extern struct Env_list env_sched_list[2]; // runnable env list

void env_init(void);
int env_alloc(struct Env **e, u_int parent_id);
void env_free(struct Env *);
void env_create_priority(u_char *binary, int size, int priority);
void env_create(u_char *binary, int size);
void env_destroy(struct Env *e);

int envid2env(u_int envid, struct Env **penv, int checkperm);
void env_run(struct Env *e);

/*challenge*/
LIST_HEAD(Thread_list, Thread);
// variable declaration
extern struct Thread *curthread;
extern struct Thread_list thread_sched_list[2];


// function decleration
int id2thread(u_int thread_id, struct Thread **pthread);
int thread_alloc(struct Env *e, struct Thread **new);
void thread_free(struct Thread *t);
void thread_destroy(struct Thread *t);
void thread_run(struct Thread *t);

// for the grading script
#define ENV_CREATE2(x, y) \
{ \
	extern u_char x[], y[]; \
	env_create(x, (int)y); \
}
#define ENV_CREATE_PRIORITY(x, y) \
{\
        extern u_char binary_##x##_start[]; \
        extern u_int binary_##x##_size;\
        env_create_priority(binary_##x##_start, \
                (u_int)binary_##x##_size, y);\
}
#define ENV_CREATE(x) \
{ \
	extern u_char binary_##x##_start[];\
	extern u_int binary_##x##_size; \
	env_create(binary_##x##_start, \
		(u_int)binary_##x##_size); \
}

#endif // !_ENV_H_
