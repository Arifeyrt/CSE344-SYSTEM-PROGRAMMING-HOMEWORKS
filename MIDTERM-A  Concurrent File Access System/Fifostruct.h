#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SERVER_FIFO "/tmp/neHos_sv.%ld"
#define CLIENT_FIFO "/tmp/neHos_cl.%ld"
#define SERVER_FIFO_LEN 100
#define CLIENT_FIFO_LEN 100
#define MESSAGE_LEN 1000

struct request 
{
    pid_t pid;
    char command[100];
};

struct response 
{
    char message[MESSAGE_LEN];
};