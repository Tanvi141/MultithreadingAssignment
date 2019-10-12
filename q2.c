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

typedef struct chefs
{
    int number; //number in the array
    int prepared; //number of prepared vessels          
} chefs;

typedef struct tables
{
    int capacity; //number of student it can serve in total
    int serving;  //0 if it has no vessel, 1 if it does
    pthread_cond_t cond;
    pthread_mutex_t mutex;
} tables;

typedef struct students
{
    int waiting; //0 means not arrived, 1 means waiting to eat, 2 means eating or done eating
    int time_arrive; //when they arrive
    int table_eating; //what table it got a slot at. -1 if no table assigned
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} students;

int n_tables, m_chefs, k_students;
int studentcount; //0 when all have eaten
pthread_t chefthreads[100];
pthread_t studentthreads[100];
pthread_t tablethreads[100];
chefs *cheftrack[100];
tables *tabletrack[100];
students *studenttrack[100];

void *thread_table(void *arg)
{
    int input = *(int *)arg;
    pthread_mutex_lock(&tabletrack[input]->mutex);
    //now table is given a container by robot
    tabletrack[input]->capacity = 25 + rand() % 26;
    pthread_mutex_unlock(&tabletrack[input]->mutex);

    while (k_students > 0 && tabletrack[input]->capacity > 0)
    {
        ready_to_serve_table(input);
    }

    return NULL;
}

int ready_to_serve_table(int num)
{
    int slots = 1 + rand() % 10;
    if (slots > tabletrack[num]->capacity)
        slots = tabletrack[num]->capacity;
    tabletrack[num]->capacity -= slots;
    printf("Table %d made %d slots available", num, slots);

    int flag = 1; //if flag is k_students that means no student is waiting
    while (slots > 0 && flag < k_students)
    {
        flag = 0;
        for (int i = 0; i < k_students; i++)
        {
            if (studenttrack[i]->waiting == 1)
            {
                int s = pthread_mutex_trylock(&studenttrack[i]->mutex);
                if (s == 0)
                {
                    if (studenttrack[i]->waiting == 1)
                    {
                        studenttrack[i]->waiting = 2;
                        studentcount--;
                        pthread_mutex_unlock(&studenttrack[i]->mutex);
                        pthread_cond_signal(&studenttrack[num]->cond);
                        slots--;
                    }
                }
            }

            else
                flag++;
        }
    }

    for (int i = 0; i < k_students; i++)
    {
        if(studenttrack[i]->table_eating==num) printf("Student %d eating at table %d\n",i,num);
    }
    return 0;
}

void *thread_student(void *arg)
{
    int num = *(int *)arg;
    sleep(studenttrack[num]->time_arrive);

    pthread_mutex_lock(&studenttrack[num]->mutex);
    studenttrack[num]->waiting = 1;
    pthread_mutex_unlock(&studenttrack[num]->mutex);

    printf("Student %d arrived at mess and waiting for slot", num);
    wait_for_slot(num);
    printf("Student %d allocated a slot at table %d", num, studenttrack[num]->table_eating); //this is the student in slot fuction
    //starts eating in the table thread
    return NULL;
}

int wait_for_slot(int num)
{
    pthread_cond_wait(&studenttrack[num]->cond, &studenttrack[num]->mutex);
    pthread_mutex_unlock(&studenttrack[num]->mutex);
}

void *thread_chef(void *arg)
{
    int input = *(int *)arg;
    while (studentcount > 0)
    {
        int w_preparetime = 2 + rand() % 3;
        sleep(w_preparetime);

        int r_vessels = 1 + rand() % 10;

        cheftrack[input]->prepared = r_vessels;

        biryani_ready(input);
    }
    return NULL;
}

int biryani_ready(int num)
{
    while (studentcount > 0 && cheftrack[num]->prepared > 0)
    {
        for (int i = 0; i < n_tables; i++)
        {
            if (tabletrack[i]->serving == 0)
            {
                int s = pthread_mutex_trylock(&tabletrack[i]->mutex);
                if (s == 0)
                {
                    if (tabletrack[i]->serving == 0)
                    {
                        tabletrack[i]->serving = 1;
                        pthread_mutex_unlock(&tabletrack[i]->mutex);
                        pthread_create(&tablethreads[i], NULL, thread_table, (void *)(i));
                        cheftrack[num]->prepared--;
                    }
                }
            }
        }
    }
    return;
}

int main()
{
    srand(time(0));
    scanf("%d%d%d", &n_tables, &m_chefs, &k_students);

    for (int i = 0; i < m_chefs; i++)
    {
        cheftrack[i] = (struct chefs *)malloc(sizeof(struct chefs));
        cheftrack[i]->prepared = 0;
        cheftrack[i]->number = i;
    }

    for (int i = 0; i < n_tables; i++)
    {
        tabletrack[i] = (struct tables *)malloc(sizeof(struct tables));
        tabletrack[i]->serving = 0;
    }

    for (int i = 0; i < k_students; i++)
    {
        studenttrack[i] = (struct students *)malloc(sizeof(struct students));
        studenttrack[i]->table_eating=-1;
    }

    //creating threads

    for (int i = 0; i < m_chefs; i++)
    {
        pthread_create(&chefthreads[i], NULL, thread_chef, (void *)(i));
    }

    for (int i = 0; i < k_students; i++)
    {
        pthread_create(&studentthreads[i], NULL, thread_student, (void *)(i));
    }
}