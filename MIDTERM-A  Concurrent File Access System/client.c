#include "Fifostruct.h"
int wait_flag;
int sigint_flag = 0;
static char client_fifo[CLIENT_FIFO_LEN];
int exit_flag = 0; // terminate flag

void handler(int signum)
{
    if (signum == SIGINT)
    {
        exit_flag = 1;
        sigint_flag = 1;
    }
    else if (signum == SIGTERM)
    {
        exit_flag = 1;
    }
}
void handle_queue_full(struct response *res, int flag)
{
    if (strcmp(res->message, "FULL") == 0)
    {

        if (flag != 2)
        {
            char temp[50];
            sprintf(temp, "/%ld", (long)getpid());
            sem_t *sem = sem_open(temp, O_CREAT, 0644, 0);
            if (sem == SEM_FAILED)
            {
                perror("sem_open failed "); // sem_open failed
                exit(1);
            }

            if (sem_wait(sem) == -1)
            {
                if (exit_flag == 1)
                {
                    printf("Server is terminated, bye\n");
                }
                else
                {
                    perror("sem_wait failed");
                    exit(1);
                }
            }

            if (exit_flag == 0)
                wait_flag = 1;

            if (sem_close(sem) == -1)
            {
                perror("close writer error");
                exit(1);
            }
            if (sem_unlink(temp) == -1)
            {
                perror("unlink writer error");
                exit(1);
            }
        }
        else
        {
            printf("Que is full. Terminating...\n");
            fflush(stdout);
        }
    }
}
void handle_queue_not_full(struct response *res, int server_fd, struct request *req, int client_fd)
{
    char input[100];
    if (strcmp(res->message, "ADD") == 0 || wait_flag == 1)
    {
        printf("Connection established.\n");
        fflush(stdout);

        while (exit_flag == 0)
        {
            // get the command from the user
            printf(">>Enter the command: ");
            fflush(stdout);

            // get the command from the user
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = '\0';
            strcpy(req->command, input);

            /* if SIGTERM is received, then break the loop */
            if (exit_flag == 1)
            {
                if (sigint_flag == 0)
                {
                    printf("\nServer is terminated, bye \n");
                    fflush(stdout);
                }
                break;
            }

            // write the request to server fifo
            if (write(server_fd, req, sizeof(struct request)) != sizeof(struct request))
            {
                perror("write to server error");
                exit(1);
            }

            client_fd = open(client_fifo, O_RDONLY);
            if (client_fd == -1)
            {
                perror("client_fd open error");
                exit(1);
            }

            /* read the response from client fifo */
            if (read(client_fd, res, sizeof(struct response)) != sizeof(struct response))
            {
                if (errno != EAGAIN)
                {
                    perror("error reading from client");
                    exit(1);
                }
            }

            if (close(client_fd) == -1)
            {
                perror("client_fd close error");
                exit(1);
            }

            printf("%s\n", res->message);
            fflush(stdout);
            // break the loop to terminate the program
            if (strcmp(input, "killServer") == 0 || strcmp(input, "quit") == 0)
                break;
        }
    }
}
void create_client_fifo()
{
    umask(0);
    snprintf(client_fifo, CLIENT_FIFO_LEN, CLIENT_FIFO, (long)getpid());
    if (mkfifo(client_fifo, S_IWUSR | S_IRUSR | S_IWGRP) == -1 && errno != EEXIST)
    {
        perror("mkfifo error");
        exit(1);
    }
}

void operate_client(const char serverpid[], int flag)
{
    int server_fd, client_fd;
    struct request req;
    struct response res;
    char server_fifo[SERVER_FIFO_LEN];

    // create the client fifo
    create_client_fifo();
    req.pid = getpid();

    snprintf(server_fifo, SERVER_FIFO_LEN, SERVER_FIFO, (long)atoi(serverpid));
    server_fd = open(server_fifo, O_WRONLY);
    if (server_fd == -1)
    {
        perror("server_fd open error");
        exit(1);
    }

    // set the flag to the request
    char str_flag[2];
    sprintf(str_flag, "%d", flag);
    strcpy(req.command, str_flag);

    // write the request to server fifo
    if (write(server_fd, &req, sizeof(struct request)) != sizeof(struct request))
    {
        perror("write to server error");
        exit(1);
    }

    client_fd = open(client_fifo, O_RDONLY);
    if (client_fd == -1)
    {
        perror("client_fd open error");
        exit(1);
    }

    // read the response from client fifo
    if (read(client_fd, &res, sizeof(struct response)) != sizeof(struct response))
    {
        if (errno != EAGAIN)
        {
            perror("error reading from client");
            exit(1);
        }
    }

    if (close(client_fd) == -1)
    {
        perror("client_fd close error");
        exit(1);
    }

    handle_queue_full(&res, flag);
    handle_queue_not_full(&res, server_fd, &req, client_fd);

    /* if SIGINT is received, then send a quit command to server so that it removes the client */
    if (sigint_flag == 1)
    {
        strcpy(req.command, "quit");
        if (write(server_fd, &req, sizeof(struct request)) != sizeof(struct request))
        {
            perror("write to server error");
            exit(1);
        }

        client_fd = open(client_fifo, O_RDONLY);
        if (client_fd == -1)
        {
            perror("client_fd open error");
            exit(1);
        }

        /* read the response from client fifo */
        if (read(client_fd, &res, sizeof(struct response)) != sizeof(struct response))
        {
            if (errno != EAGAIN)
            {
                perror("error reading from client");
                exit(1);
            }
        }

        if (close(client_fd) == -1)
        {
            perror("client_fd close error");
            exit(1);
        }

        printf("%s\n", res.message);
        fflush(stdout);
    }

    if (close(server_fd) == -1)
    {
        perror("Error closing server FIFO");
        exit(1);
    }

    unlink(client_fifo);

    exit(0);
}

int isValidNumber(char const str_flag[])
{
    int i = 0;
    while (str_flag[i] != '\0')
    {
        if (isdigit(str_flag[i]) == 0)
            return 0;
        i++;
    }
    return 1;
}

int main(int argc, char const *argv[])
{

    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    int flag = 0;

    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        perror("sigaction error");
        exit(1);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction error");
        exit(1);
    }

    if (argc == 3)
    {
        if (isValidNumber(argv[2]) != 1)
        {
            perror("invalid argument for server pid");
            return 0;
        }

        /*set the flag */
        if (strcmp("Connect", argv[1]) == 0)
        {
            flag = 1;
            printf(">> Waiting for Que... \n");
        }
        else if (strcmp("tryConnect", argv[1]) == 0)
        {
            flag = 2;
            printf(">> Waiting for Que... \n");
        }
        else
        {
            perror("invalid argument type");
            return 0;
        }

        operate_client(argv[2], flag);
    }
    else
    {
        perror("invalid argument number");
        return 0;
    }

    return 0;
}