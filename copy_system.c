#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define BUFFER_SIZE 4096

int main()
{
    int sourceFile = open("largefile.bin", O_RDONLY);

    if (sourceFile == -1)
    {
        perror("Cannot open source file");
        return 1;
    }

    int destinationFile = open("copy_system.bin",
                               O_WRONLY | O_CREAT | O_TRUNC,
                               0644);

    if (destinationFile == -1)
    {
        perror("Cannot create destination file");
        return 1;
    }

    char buffer[BUFFER_SIZE];

    int bytesRead;

    clock_t startTime = clock();

    while ((bytesRead = read(sourceFile, buffer, BUFFER_SIZE)) > 0)
    {
        write(destinationFile, buffer, bytesRead);
    }

    clock_t endTime = clock();

    close(sourceFile);
    close(destinationFile);

    double executionTime =
        (double)(endTime - startTime) / CLOCKS_PER_SEC;

    printf("File copied successfully.\n");
    printf("Execution time: %.4f seconds\n", executionTime);

    return 0;
}
