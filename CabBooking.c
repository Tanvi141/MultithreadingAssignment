#define _POSIX_C_SOURCE 199309L //required for clock
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <wait.h>
#include <sys/shm.h>

typedef struct rider
{
    int time_ride;
    int time_arrive;
    int time_wait;
    int type;
    int number;
} rider;

typedef struct cab
{
    int status; //0 means free, 1 means premier, 2 means poolone, 3 means poolfull

} cab;

sem_t freecabs, waitings;
pthread_mutex_t lock, lock2;
int n_cabs, m_riders;
cab cabtrack[100];           //0 based indexing
pthread_t riderthreads[100]; //0 based indexing
pthread_t riderthreadsnew[100];
int riderflag[100];

void *thread_checker(void *arg)
{
    rider *input = (rider *)arg;

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return NULL; //error handliing!!!!!
    ts.tv_sec += input->time_wait;

    int s;
    //printf("Rider %d is here\n", input->number);
    while ((s = sem_timedwait(&freecabs, &ts)) == -1 && errno == EINTR)
        continue;
    //printf("Rider %d is here now with riderflag %d\n", input->number, riderflag[input->number]);
    if (riderflag[input->number] == -1) //means found an empty seat
    {
        if (s != -1)
            sem_post(&freecabs);
        return NULL;
    }

    if (s == -1)
    {
        riderflag[input->number] = -2; //means timeout
        if (errno == ETIMEDOUT)
        {
            printf("\033[1;31m"
                   "TIMEOUT: Unable to find cab for rider %d\n"
                   "\033[0m",
                   input->number);
        }
        else
            perror("sem_timedwait");
    }
    else
    {
        riderflag[input->number] = 1;
    }
    return NULL;
}

void *thread_pool(void *arg)
{
    rider *input = (rider *)arg;
    sleep(input->time_arrive);

    int i, joinflag = 0;

    pthread_mutex_lock(&lock);
    for (i = 0; i < n_cabs; i++)
    {
        if (cabtrack[i].status == 2) //can join an existing cab
        {
            cabtrack[i].status = 3;
            break;
        }
    }

    if (i >= n_cabs) //needs a new cab or a new seat
    {
        pthread_mutex_unlock(&lock);
        joinflag = 1;

        pthread_create(&riderthreadsnew[i], NULL, thread_checker, (void *)(input));

        pthread_mutex_lock(&lock2);
        while (riderflag[input->number] == 0)
        {
            for (i = 0; i < n_cabs; i++)
            {
                if (cabtrack[i].status == 2)
                {
                    pthread_mutex_lock(&lock);
                    riderflag[input->number] = -1; //can join an existing cab
                    cabtrack[i].status = 3;
                    //printf("Rider %d found cab %d free\n", input->number, i);
                    break;
                }
            }
        }
        //printf("Rider flag for %d is %d\n", input->number, riderflag[input->number]);
        pthread_mutex_unlock(&lock2);

        if (riderflag[input->number] != -1)
            pthread_mutex_lock(&lock);
        //printf("Rider flag for %d is %d\n", input->number, riderflag[input->number]);
        if (riderflag[input->number] == -2) //means timeout
        {
            pthread_mutex_unlock(&lock);
            pthread_join(riderthreadsnew[input->number], NULL);
            //printf("Yah rider %d thread is joined\n", input->number);
            return NULL;
        }
        else if (riderflag[input->number] == 1) //means found a free cab
        {
            //printf("Rider flag for %d is %d\n", input->number, riderflag[input->number]);
            for (i = 0; i < n_cabs; i++)
            {
                if (cabtrack[i].status == 0)
                {
                    cabtrack[i].status = 2; //can join an existing cab
                    break;
                }
            }
        }
        //printf("Rider %d found cab %d free\n", input->number, i);
    }
    pthread_mutex_unlock(&lock);

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
        cabtrack[i].status = 2;
    }
    else if (cabtrack[i].status == 2)
    {
        sem_post(&freecabs);
        cabtrack[i].status = 0;
    }
    else
    {
        printf("This should not be happening\n");
        printf("Status of cab %d from rider %d is %d lol why\n", i, input->number, cabtrack[i].status);
    }
    pthread_mutex_unlock(&lock);
    if (joinflag == 1)
    {
        pthread_join(riderthreadsnew[input->number], NULL);
        //printf("Yah rider %d thread is joined\n", input->number);
    }
    return NULL;
}

void *thread_premier(void *arg)
{
    rider *input = (rider *)arg;

    sleep(input->time_arrive);

    sem_wait(&freecabs); //change

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
    cabtrack[i].status = 0;
    pthread_mutex_unlock(&lock);

    sem_post(&freecabs);
    return NULL;
}

int main()
{
    scanf("%d%d", &n_cabs, &m_riders);
    sem_init(&freecabs, 0, n_cabs);

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
        riderflag[i] = 0;
        ridertrack[i] = (struct rider *)malloc(sizeof(struct rider));
        ridertrack[i]->number = i;
        //1 if poolcab
        //0 if premier
        scanf("%d%d%d%d", &ridertrack[i]->type, &ridertrack[i]->time_arrive, &ridertrack[i]->time_ride, &ridertrack[i]->time_wait);
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
    return 0;
}

// 1 4
// 1 0 3 10
// 1 0 3 10
// 1 0 3 10
// 1 0 3 10
