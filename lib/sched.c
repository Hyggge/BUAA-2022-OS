#include <env.h>
#include <pmap.h>
#include <printf.h>

/* Overview:
 *  Implement simple round-robin scheduling.
 *
 *
 * Hints:
 *  1. The variable which is for counting should be defined as 'static'.
 *  2. Use variable 'env_sched_list', which is a pointer array.
 *  3. CANNOT use `return` statement!
 */
/*** exercise 3.15 ***/

extern struct Thread* curthread;
extern struct Thread_list thread_sched_list[];


void sched_yield(void)
{

    static int count = 0;
    static int point = 0;
    
    struct Thread *t = curthread;

    if (count == 0 || t == NULL || t->thread_status != THREAD_RUNNABLE) {
       	if (t != NULL) {
			LIST_REMOVE(t, thread_sched_link);
        	LIST_INSERT_TAIL(&thread_sched_list[1-point], t, thread_sched_link);
        }
        while(1) {
            if (LIST_EMPTY(&thread_sched_list[point])) {
                point = 1 - point;
            }
            
            t = LIST_FIRST(&thread_sched_list[point]);

            if (t->thread_status == THREAD_RUNNABLE) {
                break;
            } 
            else {
                LIST_REMOVE(t, thread_sched_link);
                LIST_INSERT_TAIL(&thread_sched_list[1-point], t, thread_sched_link);
            }
        }
		count = t->thread_pri;
    }

    count--;
    thread_run(t);

}

