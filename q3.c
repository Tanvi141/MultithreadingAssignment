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

} cab;

sem_t freecabs, waitings;
int pool_seats_available;
pthread_mutex_t lock, lock2;
int n_cabs, m_riders;
cab cabtrack[100]; //0 based indexing

void *thread_pool(void *arg)
{
    rider *input = (rider *)arg;
    sleep(input->time_arrive);

    int i;

    //printf("Rider %d left the while because poolvalue was %d\n", input->number, pool_seats_available);

    
    if (pool_seats_available != 0)
    {
        pthread_mutex_lock(&lock);
        //printf("Rider %d here", input->number);
        pool_seats_available -= 1;
        for (i = 0; i < n_cabs; i++)
        {
            if (cabtrack[i].status == 2)
            {
                cabtrack[i].status = 3;
                break;
            }
        }
        pthread_mutex_unlock(&lock);
    }

    else
    {
    
        //printf("Rider %d her e", input->number);
        //pthread_mutex_unlock(&lock);
        //sem_wait(&freecabs);
        pthread_mutex_lock(&lock);
        pool_seats_available += 1;
        //sem_post(&waitings);
        for (i = 0; i < n_cabs; i++)
        {
            if (cabtrack[i].status == 0)
            {
                cabtrack[i].status = 2;
                break;
            }
        }
        pthread_mutex_unlock(&lock);
    }

    printf("\x1B[34m"
           "Rider %d riding in cab %d\n"
           "\x1B[0m",
           input->number, i);

    sleep(input->time_ride); //riding

    printf("\x1b[36m"
           "Rider %d exited from cab %d\n"
           "\x1B[0m",
           input->number, i);

    pthread_mutex_lock(&lock);
    if (cabtrack[i].status == 3)
    {
        pool_seats_available += 1;
        cabtrack[i].status = 2;
    }
    else if (cabtrack[i].status == 2)
    {
        pool_seats_available -= 1;
        sem_post(&freecabs);
        cabtrack[i].status = 0;
    }
    else
    {
        printf("This should not be happening\n");
        printf("Status of cab %d is %d lol why\n", i, cabtrack[i].status);
    }
    pthread_mutex_unlock(&lock);

    return NULL;
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

    printf("\x1B[33m"
           "Rider %d riding in cab %d\n"
           "\x1B[0m",
           input->number, i);

    sleep(input->time_ride); //riding

    printf("\x1b[35m"
           "Rider %d exited from cab %d\n"
           "\x1B[0m",
           input->number, i);

    pthread_mutex_lock(&lock);
    cabtrack[i].status = 1;
    pthread_mutex_unlock(&lock);

    sem_post(&freecabs);
    return NULL;
}

int main()
{
    pool_seats_available = 0;
    scanf("%d%d", &n_cabs, &m_riders);
    sem_init(&freecabs, 0, n_cabs);
    sem_init(&waitings, 0, 1);

    pthread_t riderthreads[m_riders]; //0 based indexing

    rider *ridertrack[m_riders];

    //intitalising
    for (int i = 0; i < n_cabs + 1; i++)
    {
        cabtrack[i].status = 0;
        // cabtrack[i].seat1 = 0;
        // cabtrack[i].seat2 = 0;
    }

    for (int i = 0; i < m_riders; i++)
    {
        ridertrack[i] = (struct rider *)malloc(sizeof(struct rider));
        ridertrack[i]->number = i;
        //1 if poolcab
        //0 if premier
        scanf("%d%d%d", &ridertrack[i]->type, &ridertrack[i]->time_arrive, &ridertrack[i]->time_ride);
    }

    for (int i = 0; i < m_riders; i++)
    {
        if (ridertrack[i]->type == 0)
            pthread_create(&riderthreads[i], NULL, thread_premier, (void *)(ridertrack[i]));
        else
            pthread_create(&riderthreads[i], NULL, thread_pool, (void *)(ridertrack[i]));
    }

    for (int i = 0; i < m_riders; i++)
        pthread_join(riderthreads[i], NULL);

    sem_destroy(&freecabs);
    sem_destroy(&waitings);
    return 0;
}