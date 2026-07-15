#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

pthread_mutex_t fileLock;

struct ThreadData
{
    char *keyword;
    char *fileName;
    FILE *outputFile;
};

void *searchFile(void *argument)
{
    struct ThreadData *data = (struct ThreadData *)argument;

    FILE *inputFile = fopen(data->fileName, "r");

    if (inputFile == NULL)
    {
        printf("Cannot open %s\n", data->fileName);
        return NULL;
    }

    char word[100];
    int occurrences = 0;

    while (fscanf(inputFile, "%99s", word) == 1)
    {
        if (strcmp(word, data->keyword) == 0)
        {
            occurrences++;
        }
    }

    fclose(inputFile);

    pthread_mutex_lock(&fileLock);

    fprintf(data->outputFile,
            "%s : %d occurrences\n",
            data->fileName,
            occurrences);

    pthread_mutex_unlock(&fileLock);

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        printf("Usage:\n");
        printf("./search keyword output.txt file1.txt file2.txt ... number_of_threads\n");
        return 1;
    }

    char *keyword = argv[1];

    char *outputName = argv[2];

    int threadCount = atoi(argv[argc - 1]);

    int fileCount = argc - 4;

    if (threadCount > fileCount)
    {
        threadCount = fileCount;
    }

    if (threadCount < 1)
    {
        threadCount = 1;
    }

    FILE *outputFile = fopen(outputName, "w");

    if (outputFile == NULL)
    {
        printf("Cannot create output file\n");
        return 1;
    }

    pthread_mutex_init(&fileLock, NULL);

    pthread_t threads[threadCount];

    struct ThreadData files[threadCount];

    clock_t startTime = clock();

    int filesStarted = 0;

    while (filesStarted < fileCount)
    {
        int batchSize = threadCount;

        if (filesStarted + batchSize > fileCount)
        {
            batchSize = fileCount - filesStarted;
        }

        for (int i = 0; i < batchSize; i++)
        {
            files[i].keyword = keyword;
            files[i].fileName = argv[filesStarted + i + 3];
            files[i].outputFile = outputFile;

            pthread_create(&threads[i],
                           NULL,
                           searchFile,
                           &files[i]);
        }

        for (int i = 0; i < batchSize; i++)
        {
            pthread_join(threads[i], NULL);
        }

        filesStarted += batchSize;
    }

    clock_t endTime = clock();

    pthread_mutex_destroy(&fileLock);

    fclose(outputFile);

    double executionTime =
        (double)(endTime - startTime) / CLOCKS_PER_SEC;

    printf("Search completed successfully.\n");
    printf("Execution time: %.4f seconds\n", executionTime);

    return 0;
}
