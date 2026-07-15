#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define FILE_NAME "output.txt"

int main()
{
    int pipeEnds[2];

    pipe(pipeEnds);

    pid_t senderPid = fork();

    if (senderPid == 0)
    {
        // First child runs: ps aux

        dup2(pipeEnds[1], STDOUT_FILENO);

        close(pipeEnds[0]);
        close(pipeEnds[1]);

        char *processCommand[] = {"ps", "aux", NULL};

        execvp(processCommand[0], processCommand);

        perror("execvp");
        exit(1);
    }

    pid_t filterPid = fork();

    if (filterPid == 0)
    {
        // Second child runs: grep root

        int outputFile = open(FILE_NAME,
                              O_WRONLY | O_CREAT | O_TRUNC,
                              0644);

        dup2(pipeEnds[0], STDIN_FILENO);
        dup2(outputFile, STDOUT_FILENO);

        close(pipeEnds[0]);
        close(pipeEnds[1]);
        close(outputFile);

        char *filterCommand[] = {"grep", "root", NULL};

        execvp(filterCommand[0], filterCommand);

        perror("execvp");
        exit(1);
    }

    // Parent

    close(pipeEnds[0]);
    close(pipeEnds[1]);

    wait(NULL);
    wait(NULL);

    int inputFile = open(FILE_NAME, O_RDONLY);

    char preview[300];

    int bytesRead = read(inputFile, preview, sizeof(preview) - 1);

    preview[bytesRead] = '\0';

    printf("Output from file:\n");
    printf("%s", preview);

    close(inputFile);

    return 0;
}
