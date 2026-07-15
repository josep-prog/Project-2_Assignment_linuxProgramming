#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BUFFER_SIZE 4096

int main()
{
    FILE *sourceFile = fopen("largefile.bin", "rb");

    if (sourceFile == NULL)
    {
        perror("Cannot open source file");
        return 1;
    }

    FILE *destinationFile = fopen("copy_stdio.bin", "wb");

    if (destinationFile == NULL)
    {
        perror("Cannot create destination file");
        return 1;
    }

    char buffer[BUFFER_SIZE];

    size_t bytesRead;

    clock_t startTime = clock();

    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, sourceFile)) > 0)
    {
        fwrite(buffer, 1, bytesRead, destinationFile);
    }

    clock_t endTime = clock();

    fclose(sourceFile);
    fclose(destinationFile);

    double executionTime =
        (double)(endTime - startTime) / CLOCKS_PER_SEC;

    printf("File copied successfully.\n");
    printf("Execution time: %.4f seconds\n", executionTime);

    return 0;
}
