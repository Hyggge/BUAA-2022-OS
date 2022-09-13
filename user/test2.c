#include "lib.h"

pthread_t t1;
pthread_t t2;

void *print_message_function( void *ptr )
{
    int id = pthread_self();
    printf("\033[0;32;40m thread %d received : '%s' \033[0m\n", id, (char *)ptr);

    printf("\033[0;34;40m before `pthread_exit` ...\033[0m\n", id);
    if (id == t1) pthread_exit(3);
    else pthread_exit(4);
    printf("\033[0;34;40m after `pthread_exit` ...\033[0m\n");

    if (id == t1) return 1;
    else return 2;
}

umain()
{
    char *message1 = "I love BUAA!";
    char *message2 = "I love CS!";
    int  ret1, ret2;

    pthread_create( &t1, NULL, print_message_function, (void*) message1);
    pthread_create( &t2, NULL, print_message_function, (void*) message2);

    pthread_join( t1, &ret1);
    pthread_join( t2, &ret2); 
    
    printf("\033[0;32;40m thread %d returns: %d \033[0m\n", t1, ret1);
    printf("\033[0;32;40m thread %d returns: %d \033[0m\n", t2, ret2);
}