#include "lib.h"

pthread_t t1;
pthread_t t2;

void *fun1(void *arg) {
    printf("\033[0;34;40mreciving '%s'\033[0m\n", (char *)arg);
        int oldtype;
        pthread_setcanceltype(THREAD_CANCEL_ASYNCHRONOUS, &oldtype);
        printf("\033[0;32;40mthread %d old_cancel_ype is '%d', new_cancel_type is '%d'\033[0m\n", 
                    pthread_self(), oldtype, THREAD_CANCEL_ASYNCHRONOUS);

    int i;
    for (i = 0; i < 10; i++) {
        if (i == 5) {
            pthread_cancel(pthread_self());
        }
        printf("\033[0;34;40m %d \033[0m\n", i);
    }
    printf("\033[0;34;40m fun1 end !!!\033[0m\n");
}

void umain()
{
    char *str = "hello!";
    int ret1 = 0;
    int ret2 = 0;

    pthread_create(&t1, NULL, fun1, str);
    pthread_create(&t2, NULL, fun1, str);

    pthread_join(t1, &ret1);
    pthread_join(t2, &ret2);

    writef("\033[0;34;40m t1 return the value of: %d\033[0m\n", ret1);
    writef("\033[0;34;40m t2 return the value of: %d\033[0m\n", ret2);

    if (ret1 == PTHREAD_CANCELED) {
        writef("\033[0;32;40m t1 was canceled successfully!\033[0m\n");
    } else {
        writef("\033[0;31;40m t1 return with wrong value!\033[0m\n");
    }

    if (ret2 == PTHREAD_CANCELED) {
        writef("\033[0;32;40m t2 was canceled successfully!\033[0m\n");
    } else {
        writef("\033[0;31;40m t2 return with wrong value!\033[0m\n");
    }
}
