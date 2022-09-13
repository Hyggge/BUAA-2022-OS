#include "lib.h"

pthread_t t1, t2;
sem_t mutex, empty, full;
int max = 1;
int count = 0;

void *prodecer(void *arg) {
    int i;
    for(i = 0; i < 100; i++) {
        sem_wait(&empty);
        sem_wait(&mutex);
        count++;
        printf("\033[0;32;40m produce successfully, no count is %d \033[0m\n", count);
        sem_post(&mutex);
        sem_post(&full);
    }
}

void *consumer(void *arg) {
    int i;
    for(i = 0; i < 100; i++) {
        sem_wait(&full);
        sem_wait(&mutex);
        count--;
        printf("\033[0;31;40m consume successfully, no count is %d \033[0m\n", count);
        sem_post(&mutex);
        sem_post(&empty);
    }
}

void umain()
{   
    sem_init(&mutex, 0, 1);
    sem_init(&empty, 0, max);
    sem_init(&full, 0, 0);
    
    pthread_create(&t1, NULL, prodecer, NULL);
    pthread_create(&t2, NULL, consumer, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
}