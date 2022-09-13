#include "lib.h"
#include <mmu.h>
#include <env.h>

extern int get_retval();

void
exit(void)
{
//	writef("enter exit!");
	//close_all();
	void *retval = get_retval();
	int thread_id = syscall_get_thread_id();
	struct Thread *cur_thread = &env->env_threads[THREAD2INDEX(thread_id)];

	if (THREAD2INDEX(thread_id) == 0) {
		cur_thread->thread_retval = 0;
		syscall_thread_destroy(0);
	}
	else if ((cur_thread->thread_tag & THREAD_TAG_CANCELED) != 0) {
		syscall_thread_destroy(0);
	}
	else if ((cur_thread->thread_tag & THREAD_TAG_EXITED) != 0) {
		syscall_thread_destroy(0);
	}
	else {
		cur_thread->thread_retval = retval;
		syscall_thread_destroy(0);
	}
}


struct Env *env;

void
libmain(int argc, char **argv)
{
	// set env to point at our env structure in envs[].
	env = 0;	// Your code here.
	//writef("xxxxxxxxx %x  %x  xxxxxxxxx\n",argc,(int)argv);
	int envid;
	envid = syscall_getenvid();
	envid = ENVX(envid);
	env = &envs[envid];
	// call user main routine
	umain(argc, argv);
	// exit gracefully
	exit();
	//syscall_env_destroy(0);
}
