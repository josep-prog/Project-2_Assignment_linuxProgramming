#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define THREADS 16
#define MAX_NUMBER 200000

int totalPrimes = 0;

pthread_mutex_t counterLock;

struct ThreadData
{
    int start;
    int end;
};

int isPrime(int number)
{
    if (number < 2)
        return 0;

    for (int i = 2; i * i <= number; i++)
    {
        if (number % i == 0)
            return 0;
    }

    return 1;
}

void *countPrimes(void *argument)
{
    struct ThreadData *data = (struct ThreadData *)argument;

    int localCount = 0;

    for (int number = data->start; number <= data->end; number++)
    {
        if (isPrime(number))
        {
            localCount++;
        }
    }

    pthread_mutex_lock(&counterLock);

    totalPrimes += localCount;

    pthread_mutex_unlock(&counterLock);

    return NULL;
}

int main()
{
    pthread_t threads[THREADS];

    struct ThreadData ranges[THREADS];

    pthread_mutex_init(&counterLock, NULL);

    int numbersPerThread = MAX_NUMBER / THREADS;

    for (int i = 0; i < THREADS; i++)
    {
        ranges[i].start = i * numbersPerThread + 1;

        ranges[i].end = (i + 1) * numbersPerThread;

        if (i == THREADS - 1)
        {
            ranges[i].end = MAX_NUMBER;
        }

        pthread_create(&threads[i],
                       NULL,
                       countPrimes,
                       &ranges[i]);
    }

    for (int i = 0; i < THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&counterLock);

    printf("The synchronized total number of prime numbers between 1 and 200000 is %d\n",
           totalPrimes);

    return 0;
}
