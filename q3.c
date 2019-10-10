//source: https://www.geeksforgeeks.org/use-posix-semaphores-c/

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct rider
{
    int time_ride;
    int time_arrive;
    int type;
    int number;
} rider;

typedef struct cab
{
    int status; //0 means free, 1 means premier, 2 means poolone, 3 means poolfull
    int seat1;
    int seat2;
} cab;

sem_t freecabs;
sem_t waitingriders;
int poolcab;
pthread_mutex_t lock;
int n_cabs, m_riders;
cab cabtrack[100]; //0 based indexing

void *thread(void *arg)
{
    //wait
    sem_wait(&freecabs);
    printf("\nEntered..\n");

    //critical section
    sleep(4);

    //signal
    printf("\nJust Exiting...\n");
}

void *thread_premier(void *arg)
{
    rider *input = (rider *)arg;

    sleep(input->time_arrive);

    sem_wait(&freecabs);

    pthread_mutex_lock(&lock);
    int i;
    for (i = 0; i < n_cabs; i++)
    {
        if (cabtrack[i].status == 0)
        {
            cabtrack[i].status = 1;
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    printf("\x1B[31m"
           "Rider %d riding in cab %d\n"
           "\x1B[0m",
           input->number, i);
    sleep(input->time_ride);

    pthread_mutex_lock(&lock);
    cabtrack[i].status = 1;
    pthread_mutex_unlock(&lock);

    sem_post(&freecabs);
    printf("\x1b[35m"
           "Rider %d exited from cab %d\n"
           "\x1B[0m",
           input->number, i);
    return NULL;
}

int main()
{
    poolcab = 0;
    scanf("%d%d", &n_cabs, &m_riders);
    sem_init(&freecabs, 0, n_cabs);

    pthread_t riderthreads[m_riders]; //0 based indexing

    rider *ridertrack[m_riders];

    //intitalising
    for (int i = 0; i < n_cabs + 1; i++)
    {
        cabtrack[i].status = 0;
        cabtrack[i].seat1 = 0;
        cabtrack[i].seat2 = 0;
    }

    for (int i = 0; i < m_riders; i++)
    {
        ridertrack[i] = (struct rider *)malloc(sizeof(struct rider));
        ridertrack[i]->number = i;
        //1 if poolcab
        //0 if premier
        scanf("%d%d", &ridertrack[i]->time_arrive, &ridertrack[i]->time_ride);
    }

    for (int i = 0; i < m_riders; i++)
        pthread_create(&riderthreads[i], NULL, thread_premier, (void *)(ridertrack[i]));

    for (int i = 0; i < m_riders; i++)
        pthread_join(riderthreads[i], NULL);

    sem_destroy(&freecabs);
    sem_destroy(&waitingriders);
    return 0;
}