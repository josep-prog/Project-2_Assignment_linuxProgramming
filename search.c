#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

pthread_mutex_t fileLock;
pthread_mutex_t indexLock;

char *keyword;
char **fileNames;
int fileCount;
int nextFileIndex;
FILE *outputFile;

/*
 * Every requested thread runs this function. Instead of pre-assigning one
 * fixed file per thread (which forces threadCount to be capped down to
 * fileCount whenever more threads are requested than there are files),
 * each thread pulls the next unclaimed file from a shared index protected
 * by indexLock. This lets threadCount stay exactly what was requested:
 * with more threads than files, the extra threads are still created and
 * joined (so their overhead is real and measurable) but simply find no
 * file left to claim and exit immediately.
 */
void *searchFiles(void *argument)
{
    (void)argument;

    while (1)
    {
        pthread_mutex_lock(&indexLock);

        int index = nextFileIndex;

        if (index < fileCount)
        {
            nextFileIndex++;
        }

        pthread_mutex_unlock(&indexLock);

        if (index >= fileCount)
        {
            break;
        }

        FILE *inputFile = fopen(fileNames[index], "r");

        if (inputFile == NULL)
        {
            printf("Cannot open %s\n", fileNames[index]);
            continue;
        }

        char word[100];
        int occurrences = 0;

        while (fscanf(inputFile, "%99s", word) == 1)
        {
            if (strcmp(word, keyword) == 0)
            {
                occurrences++;
            }
        }

        fclose(inputFile);

        pthread_mutex_lock(&fileLock);

        fprintf(outputFile,
                "%s : %d occurrences\n",
                fileNames[index],
                occurrences);

        pthread_mutex_unlock(&fileLock);
    }

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

    keyword = argv[1];

    char *outputName = argv[2];

    int threadCount = atoi(argv[argc - 1]);

    if (threadCount < 1)
    {
        threadCount = 1;
    }

    fileNames = &argv[3];
    fileCount = argc - 4;
    nextFileIndex = 0;

    outputFile = fopen(outputName, "w");

    if (outputFile == NULL)
    {
        printf("Cannot create output file\n");
        return 1;
    }

    pthread_mutex_init(&fileLock, NULL);
    pthread_mutex_init(&indexLock, NULL);

    pthread_t *threads = malloc(threadCount * sizeof(pthread_t));

    clock_t startTime = clock();

    for (int i = 0; i < threadCount; i++)
    {
        pthread_create(&threads[i], NULL, searchFiles, NULL);
    }

    for (int i = 0; i < threadCount; i++)
    {
        pthread_join(threads[i], NULL);
    }

    clock_t endTime = clock();

    free(threads);

    pthread_mutex_destroy(&fileLock);
    pthread_mutex_destroy(&indexLock);

    fclose(outputFile);

    double executionTime =
        (double)(endTime - startTime) / CLOCKS_PER_SEC;

    printf("Search completed successfully.\n");
    printf("Threads requested: %d | Files to search: %d\n", threadCount, fileCount);
    printf("Execution time: %.4f seconds\n", executionTime);

    return 0;
}
