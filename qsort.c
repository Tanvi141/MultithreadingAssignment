#define _POSIX_C_SOURCE 199309L //required for clock
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>

int *shareMem(size_t size)
{
    key_t mem_key = IPC_PRIVATE;
    int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
    return (int *)shmat(shm_id, NULL, 0);
}

struct arg
{
    int l;
    int r;
    int *arr;
};

// A utility function to swap two elements
void swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

int partition(int arr[], int low, int high)
{
    int pivot = arr[high];
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++)
    {
        if (arr[j] < pivot)
        {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

void concurrent_quickSort(int arr[], int low, int high)
{
    if (low < high)
    {
        if (high - low < 5)
        {
            for (int i = low + 1; i <= high; i++)
            {
                int value = arr[i];
                int j = i;

                while (j > low && arr[j - 1] > value)
                {
                    arr[j] = arr[j - 1];
                    j--;
                }

                arr[j] = value;
            }
        }
        // int pi = partition(arr, low, high);
        // quickSort(arr, low, pi - 1);
        // quickSort(arr, pi + 1, high);

        int pi = partition(arr, low, high);
        int pid1 = fork();
        int pid2;
        if (pid1 == 0)
        {
            //sort left half array
            concurrent_quickSort(arr, low, pi - 1);
            exit(0);
        }
        else
        {
            pid2 = fork();
            if (pid2 == 0)
            {
                //sort right half array
                concurrent_quickSort(arr, pi + 1, high);
                exit(0);
            }
            else
            {
                //wait for the right and the left half to get sorted
                int status;
                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);
            }
        }
    }
}

void *threaded_quickSort(void *a)
{
    struct arg *args = (struct arg *)a;

    int low = args->l;
    int high = args->r;
    int *arr = args->arr;

    if (low < high)
    {
        if (high - low < 5)
        {
            for (int i = low + 1; i <= high; i++)
            {
                int value = arr[i];
                int j = i;

                while (j > low && arr[j - 1] > value)
                {
                    arr[j] = arr[j - 1];
                    j--;
                }

                arr[j] = value;
            }
        }
        int pi = partition(arr, low, high);

        struct arg a1;
        a1.l = low;
        a1.r = (low + high) / 2;
        a1.arr = arr;
        pthread_t tid1;
        pthread_create(&tid1, NULL, threaded_quickSort, &a1);

        struct arg a2;
        a2.l = (low + high) / 2 + 1;
        a2.r = high;
        a2.arr = arr;
        pthread_t tid2;
        pthread_create(&tid2, NULL, threaded_quickSort, &a2);

        pthread_join(tid1, NULL);
        pthread_join(tid2, NULL);
    }
}

void normal_quickSort(int arr[], int low, int high)
{
    if (low < high)
    {
        if (high - low < 5)
        {
            for (int i = low + 1; i <= high; i++)
            {
                int value = arr[i];
                int j = i;

                while (j > low && arr[j - 1] > value)
                {
                    arr[j] = arr[j - 1];
                    j--;
                }

                arr[j] = value;
            }
        }
        int pi = partition(arr, low, high);
        normal_quickSort(arr, low, pi - 1);
        normal_quickSort(arr, pi + 1, high);
    }
}

int main()
{

    int n;
    scanf("%d", &n);
    int *arr = shareMem(sizeof(int) * (n + 1));
    int arrnorm[n];
    int arrthread[n];

    for (int i = 0; i < n; i++)
    {
        scanf("%d", &arr[i]);
        arrnorm[i] = arr[i];
        arrthread[i] = arr[i];
    }

    struct timespec ts;
    long double st;
    long double en;

    //Concurrent
    for (int i = 0; i < n; i++)
        printf("%d ", arr[i]);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec / (1e9) + ts.tv_sec;

    concurrent_quickSort(arr, 0, n - 1);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec / (1e9) + ts.tv_sec;
    printf("Concurrent time = %Lf\n", en - st);
    for (int i = 0; i < n; i++)
        printf("%d ", arr[i]);

    //Normal
    for (int i = 0; i < n; i++)
        printf("%d ", arrnorm[i]);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec / (1e9) + ts.tv_sec;

    normal_quickSort(arrnorm, 0, n - 1);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec / (1e9) + ts.tv_sec;
    printf("Normal time = %Lf\n", en - st);
    for (int i = 0; i < n; i++)
        printf("%d ", arrnorm[i]);

    //Threaded
    struct arg a;
    a.l = 0;
    a.r = n - 1;
    a.arr = arrthread;
    for (int i = 0; i < n; i++)
        printf("%d ", a.arr[i]);

    pthread_t tid;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec / (1e9) + ts.tv_sec;

    pthread_create(&tid, NULL, threaded_quickSort, &a);
    pthread_join(tid, NULL);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec / (1e9) + ts.tv_sec;
    printf("Threaded time = %Lf\n", en - st);
    for (int i = 0; i < n; i++)
        printf("%d ", a.arr[i]);

    return 0;
}
