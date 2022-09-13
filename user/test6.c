#include "lib.h"

pthread_t t1;
pthread_t t2;
pthread_t t3;

sem_t s1;

void *func(void *arg) {
    int i = 0;
    int ret = 0;
    int value = 0;
    for (i = 0; i < 5; i++) {
    ret = sem_trywait(&s1);
            sem_getvalue(&s1, &value);
    printf("\033[0;32;40m thread %d call the `sem_trywait`, retval is %d, s1's value is %d\033[0m\n", 
                pthread_self(), ret, value);
    }
    return 1;
}


void umain()
{
    int msg = "hello, world";
    int value = 0;
    
    sem_init(&s1, 0, 5);
    printf("\033[0;34;40m after init, s1 perm is %d\033[0m\n", s1.sem_perm);
    if (s1.sem_perm == SEM_PERM_VALID) 
    printf("\033[0;34;40m s1 is valid! \033[0m\n");
    
    sem_getvalue(&s1, &value);
    printf("\033[0;34;40m s1 value is %d\033[0m\n", s1.sem_value);


    pthread_create(&t1, NULL, func, msg);
    pthread_create(&t2, NULL, func, msg);


    // wait for t1
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    sem_destroy(&s1);
    printf("\033[0;34;40m s1 after destroy, perm is %d\033[0m\n", s1.sem_perm);
    if (s1.sem_perm != SEM_PERM_VALID) 
    printf("\033[0;34;40m s1 is invalid! \033[0m\n");

}
