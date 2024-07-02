#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#define FIFO1 "fifo1"
#define FIFO2 "fifo2"

int count_childexit = 0;
int arraysize = 0;

void child_handler(int sig)
{
    pid_t id;
    int status;
    while ((id = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (WIFEXITED(status))
        {
            printf("Child with PID %d exited, status=%d\n", id, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("Child with PID %d killed by signal %d\n", id, WTERMSIG(status));
        }
        count_childexit++;
    }
}
void child_process1()
{
    int fd1 = open(FIFO1, O_RDONLY);
    if (fd1 == -1)
    {
        perror("Failed to open FIFO1");
        exit(EXIT_FAILURE);
    }
    sleep(10);
    int random_numbers[arraysize];
    if (read(fd1, random_numbers, arraysize * (sizeof(int))) == -1)
    {
        perror("Error reading from FIFO1");
        exit(EXIT_FAILURE);
    }
    close(fd1);
    int sum = 0;
    for (int i = 0; i < arraysize; i++)
    {
        sum += random_numbers[i];
    }
    printf("CHILD1 SUM %d\n",sum);

    int fd2;
    fd2 = open(FIFO2, O_WRONLY);
    if (fd2 == -1)
    {
        perror("Error opening FIFO2");
        exit(EXIT_FAILURE);
    }

    if (write(fd2, &sum, sizeof(int)) == -1)
    {
        perror("Error writing to FIFO2");
        exit(EXIT_FAILURE);
    }

    close(fd2);
    exit(EXIT_SUCCESS);
}

void child_process2()
{
    int fd2 = open(FIFO2, O_RDONLY);
    if (fd2 == -1)
    {
        perror("Failed to open FIFO2");
        exit(EXIT_FAILURE);
    }
    sleep(10);

    if (fd2 == -1)
    {
        perror("Error opening FIFO2");
        exit(EXIT_FAILURE);
    }
    char commend[9];
    if (read(fd2, commend, 9) == -1)
    {
        perror("Error reading from FIFO2");
        exit(EXIT_FAILURE);
    }
    int product_numbers = 1;
    if (strcmp(commend, "multiply") != 0)
    {
        fprintf(stderr, "Error: Invalidj command\n");
        close(fd2);
        exit(EXIT_FAILURE);
    }
    else
    {
        int product_numbers = 1;
        int num;
        for (int i = 0; i < arraysize; i++)
        {
            if (read(fd2, &num, sizeof(num)) == -1)
            {
                perror("Failed to read number for multiplication");
                exit(EXIT_FAILURE);
            }
            product_numbers *= num;
        }
       printf("CHILD2 PRODUCT: %d\n", product_numbers);
        int fifo1_numbers_sum;
        if (read(fd2, &fifo1_numbers_sum, sizeof(int)) == -1)
        {
            perror("Error reading from FIFO2");
            exit(EXIT_FAILURE);
        }
        int result = fifo1_numbers_sum + product_numbers;
        printf("RESULT: %d\n", result);
    }
    close(fd2);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <array_size>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = child_handler;

    sigaction(SIGCHLD, &sa, NULL);

    if (mkfifo(FIFO1, 0666) == -1 || mkfifo(FIFO2, 0666) == -1)
    {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    arraysize = atoi(argv[1]);
    if (arraysize <= 0)
    {
        fprintf(stderr, "Array size must be greater than 0\n");
        exit(EXIT_FAILURE);
    }
    srand(time(NULL));
    int random_numbers[arraysize];
    for (int i = 0; i < arraysize; i++)
    {
        random_numbers[i] = rand() % 10;
    }

    printf("Random numbers: ");
    for (int i = 0; i < arraysize; i++)
    {
        printf("%d ", random_numbers[i]);
    }
    printf("\n");
    pid_t pid1, pid2;
    pid1 = fork();
    if (pid1 == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0)
    {
        child_process1();
    }
    else
    {
        pid2 = fork();
        if (pid2 == 0)
        {
            child_process2();
        }
        else if (pid2 == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else
        {
            int fd1, fd2;
            fd1 = open(FIFO1, O_WRONLY);
            if (fd1 == -1)
            {
                perror("Error opening FIFOs");
                exit(EXIT_FAILURE);
            }

            if (write(fd1, random_numbers, arraysize * (sizeof(int))) == -1)
            {
                perror("Error writing to FIFO1");
                exit(EXIT_FAILURE);
            }
            fd2 = open(FIFO2, O_WRONLY);
            if (fd2 == -1)
            {
                perror("Error opening FIFO2");
                exit(EXIT_FAILURE);
            }
            char *commend = "multiply";
            int len = strlen(commend);
            if (write(fd2, commend, len + 1) == -1)
            {
                perror("Error writing to FIFO1");
                exit(EXIT_FAILURE);
            }
            for (int i = 0; i < arraysize; i++)
            {
                if (write(fd2, &random_numbers[i], sizeof(int)) == -1)
                {
                    perror("Error writing to FIFO1");
                    exit(EXIT_FAILURE);
                }
            }

            while (count_childexit < 2)
            {
                printf("Proceeding...\n");
                sleep(2);
            }
            close(fd1);
            close(fd2);
            unlink(FIFO1);
            unlink(FIFO2);
            exit(EXIT_SUCCESS);
        }
    }
    return 0;
}
