#include "Fifostruct.h"

static char server_fifo[SERVER_FIFO_LEN];
int MAX_CLIENTS_NUM = 0;
pid_t clients[800];
int num_client = 0;
pid_t wait_clients[800];
int waiting_num = 0;
char log_file[100];
sem_t* sem_sem;
sem_t* server_log_sem;
int exit_flag  = 0;
sem_t* request_sem[50][2];
char file_names[50][50];
sem_t* client_sem;
int sem_number = 0;
int reader_count = 0;
char path[50];

//operations for server side  operations  are defined here
void listOperation(char output[]);
void readSpecificLine(char tokens[4][50], char res[]);
void readEntireFile(char tokens[4][50], char res[]);
void writeTWithLine(char tokens[4][50], char res[]);
void writeTWithoutLine(char tokens[4][50], char res[]);
int copy_file(const char *source, const char *destination);
void archServer(const char *sv_dir, const char *archive_name);
int uploadOperation(char path[50],char tokens[4][50] );
int downloadOperation( char path[50],char tokens[4][50]);
//int archive_op(char tokens[4][50], char path[50]);
void help_op(char command[50], char res[]);
void logFilePrint(char filename[], pid_t pid, char command[]);
int isValidNumber(char const str[]);
void serverOp();
void create_semaphores();
void close_semaphores();
void handler(int signum);
void check_que();
void operations(char input[], struct response *res);
void serverOp();
int is_here(pid_t pid, pid_t arr[], int num);

// Create all semaphores

int main(int argc, char const *argv[])
{
    /* Signals */
    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    /* set the signals */
    if(sigaction(SIGINT, &sa, NULL) == -1) 
    {
        perror("sigaction error");
        exit(1);
    }
    if(sigaction(SIGTERM, &sa, NULL) == -1) 
    {
        perror("sigaction error");
        exit(1);
    }
    if(sigaction(SIGUSR1, &sa, NULL) == -1) 
    {
        perror("sigaction error");
        exit(1);
    }

   if(argc != 3){
    printf("Usage: %s <rootAddress> <MAX_NUMBER_OF_CLIENTS>\n", argv[0]);
    exit(EXIT_FAILURE);
    }else {
        if(isValidNumber(argv[2]) == 1)
        {
            if(access(argv[1], F_OK) != 0) 
            {
                if(mkdir(argv[1], 0777) == -1) //read, write, execute allowed
                {
                    perror("creating directory error");
                    exit(1);
                }
            }
            if(chdir(argv[1]) == -1) 
            {
                perror("changing directory error");
                exit(1);
            }

            MAX_CLIENTS_NUM = atoi(argv[2]); /* set the max client number */
            printf("\nServer Started PID  %ld \n", (long) getpid());
            printf("Server is waiting for clients...\n");
            
            /* store the working directory of server */
            strcpy(path ,argv[1]);

            /* create the log file */
            sprintf(log_file, "log_%ld.txt", (long) getpid());
            int fd = open(log_file, O_WRONLY | O_CREAT, 0666);
            if (fd == -1) 
            {
                perror("file open error (log).");
                exit(1);
            }
            close(fd);

            /* start the server */
            serverOp();
        }
        else
        {
            perror("invalid argument for #clients");
            return 0;
        }
    }

    return 0;
}
void create_semaphores(){
 /* create all semaphores beforehand */
    server_log_sem = sem_open("/log", O_CREAT, 0644, 1);
    client_sem = sem_open("/client", O_CREAT, 0644, 1);
    sem_sem = sem_open("/sem", O_CREAT, 0644, 1);
    

    if(client_sem == SEM_FAILED || sem_sem == SEM_FAILED || server_log_sem == SEM_FAILED) 
    {
        perror("Failed to create semaphores");
        exit(1);
    }

    char sem1[9];
    char sem2[9];
    for(int i = 0; i < 50; i++)
    {
        sprintf(sem1, "/m%d", i);
        sprintf(sem2, "/w%d", i);
        request_sem[i][0] = sem_open(sem1, O_CREAT, 0644, 1);
        request_sem[i][1] = sem_open(sem2, O_CREAT, 0644, 1);
    }

}
// Close and unlink all semaphores
void close_semaphores(){
    char sem1[9], sem2[9];
    if(sem_close(client_sem) == -1)
    {
        perror("close writer error");
        exit(1);
    }
    if(sem_close(server_log_sem) == -1)
    {
        perror("close writer error");
        exit(1);
    }
    if(sem_close(sem_sem) == -1)
    {
        perror("close writer error");
        exit(1);
    }
    if(sem_unlink("/log") == -1)
    {
        perror("unlink writer error");
        exit(1);
    }
 
    if(sem_unlink("/sem") == -1)
    {
        perror("unlink writer error");
        exit(1);
    }
    if(sem_unlink("/client") == -1)
    {
        perror("unlink writer error");
        exit(1);
    }

    for(int i = 0; i < sem_number; i++)
    {
        if(sem_close(request_sem[i][0]) == -1)
        {
            perror("close writer error");
            exit(1);
        }
        if(sem_close(request_sem[i][1]) == -1)
        {
            perror("close writer error");
            exit(1);
        }

        sprintf(sem1, "/m%d", i);
        sprintf(sem2, "/w%d", i);

        if(sem_unlink(sem2) == -1)
        {
            perror("close writer error");
            exit(1);
        }
        if(sem_unlink(sem1) == -1)
        {
            perror("unlink writer error");
            exit(1);
        }

    }

}
/* signal handler */
void handler(int signum) {
    if (signum == SIGTERM) {
        puts("\nSIGTERM is received. Terminating...\n");
        exit_flag  = 1;
    } else if (signum == SIGUSR1) {
        exit_flag  = 1;
    } else if (signum == SIGINT) {
        puts("\nSIGINT is received. Terminating...\n");
        exit_flag  = 1;
    }
}

/* check if there is at least one client waiting */
/* if so, then add it to the client que */
void check_que()
{
   while (waiting_num != 0)
    {
        char temp[50];
        sprintf(temp, "/%ld", (long) wait_clients[0]);
        sem_t* sem = sem_open(temp, 0);
        if(sem == SEM_FAILED) 
        {
            perror("sem_open failed");
            exit(1);
        }

        /* wake up the client */
        if(sem_post(sem) == -1) 
        {
            perror("sem_post failed");
            exit(1);
        }

        clients[num_client++] = wait_clients[0];

        printf("Client PID %ld connected", (long)wait_clients[0]);
        fflush(stdout);

        for(int i = 0; i < waiting_num; i++)
        {
            if(i != waiting_num-1)
                wait_clients[i] = wait_clients[i+1];
        }

        waiting_num--;

    }
}

/* check if the given pid is in the array */
int is_here(pid_t pid, pid_t arr[], int num)
{
    for(int i = 0; i < num; i++)
    {
        if(arr[i] == pid)
            return 1;
    }
    return 0;
}

void operations(char input[], struct response *res)
{
    char temp[100];
    char tokens[4][50];
    strcpy(temp, input);//req.command is copied to temp
    int count = 0;

    char *token = strtok(temp, " ");
    while(token != NULL) 
    {
        strcpy(tokens[count], token);
        printf("%s\n", tokens[count]);
        count++;
        if(count == 4)
            break;
        token = strtok(NULL, " ");
    }

    if(strcmp( tokens[0],"help") == 0 && (count == 1 || count == 2))
    {
        if(count == 2){
            char output[MESSAGE_LEN] = "";
            help_op(tokens[1], output);
            strcpy(res->message, output);
        }
        else 
        {
        strcpy(res->message, "\nAvailable comments are : help, list, readF, writeT, upload, download,archServer, quit, killServer"); 

        }
    }
    else if(strcmp(tokens[0],"list") == 0)
    {
        char output[MESSAGE_LEN] = "";
        listOperation(output);
        strcpy(res->message, output); 
    }
    else if((strcmp("writeT", tokens[0]) == 0 && (count == 3 || count == 4))||( strcmp("readF", tokens[0]) == 0 && (count == 3 || count == 2)))
    {
        char output[MESSAGE_LEN] = "";
        int ind =-1;
        for (int i = 0; i < sem_number; i++) {
        if (strcmp(tokens[1], file_names[i]) == 0) {
        ind = i;
        break;
       }
    }
        if(ind == -1)
        {
            sem_wait(sem_sem);
            strcpy(file_names[sem_number], tokens[1]);
            ind = sem_number;
            sem_number++;
            sem_post(sem_sem);
        }

        /* reader part */
if(strcmp("readF", tokens[0]) == 0){
        if(sem_wait(request_sem[ind][0]) == -1)
           perror("sem_wait");

        reader_count++;
        if(reader_count == 1)
        {
            if(sem_wait(request_sem[ind][1]) == -1)
                perror("sem_wait");
        }

        if(sem_post(request_sem[ind][0]) == -1)
           perror("sem_post");
        
        if(count == 2){
      
         readEntireFile(tokens, output);
        }

        else if(count == 3){
          printf("tokens[2]: %s\n", tokens[1]);
          readSpecificLine(tokens, output);
        }

        //readF(tokens, output, count);

        if(sem_wait(request_sem[ind][0]) == -1)
           perror("sem_wait");

        reader_count--;

        if(reader_count == 0)
        {
            if(sem_post(request_sem[ind][1]) == -1)
                perror("sem_post");
        }

        if(sem_post(request_sem[ind][0]) == -1)
           perror("sem_post");
    }
    else if(strcmp("writeT", tokens[0]) == 0){
        
        if(sem_wait(request_sem[ind][1]) == -1)
           perror("sem_wait");

        if(count == 4)
        writeTWithLine(tokens, output);
        else
        writeTWithoutLine(tokens, output);

        if(sem_post(request_sem[ind][1]) == -1)
           perror("sem_post");

    }
        strcpy(res->message, output); 

    }
    
    else if((strcmp("download", tokens[0]) == 0 || strcmp("upload", tokens[0]) == 0) && count == 2)
    {
         int ind = -1;
         for (int i = 0; i < sem_number; i++) {
         if (strcmp(tokens[1], file_names[i]) == 0) {
         ind = i;
         break;
         }
        }
        if(ind == -1)
        {
            sem_wait(sem_sem);
            strcpy(file_names[sem_number], tokens[1]);

            ind = sem_number;
            sem_number++;

            sem_post(sem_sem);
        }
     if(strcmp("upload", tokens[0]) == 0 ){
        if(sem_wait(request_sem[ind][0]) == -1)
           perror("sem_wait");

        reader_count++;

        if(reader_count == 1)
        {
            if(sem_wait(request_sem[ind][1]) == -1)
                perror("sem_wait");
        }

        if(sem_post(request_sem[ind][0]) == -1)
           perror("sem_post");

        int res_op = uploadOperation( path,tokens);

        if(sem_wait(request_sem[ind][0]) == -1)
           perror("sem_wait");

        reader_count--;

        if(reader_count == 0)
        {
            if(sem_post(request_sem[ind][1]) == -1)
                perror("sem_post");
        }

        if(sem_post(request_sem[ind][0]) == -1)
           perror("sem_post");

        if(res_op == 0)
            strcpy(res->message, "\n file transfer request received. Beginning file transfer:"); 
        else
            strcpy(res->message, "\nFile is not found."); 
     }else if(strcmp("download", tokens[0]) == 0){
        /* reader part */
        if(sem_wait(request_sem[ind][0]) == -1)
           perror("sem_wait");

        reader_count++;

        if(reader_count == 1)
        {
            if(sem_wait(request_sem[ind][1]) == -1)
                perror("sem_wait");
        }

        if(sem_post(request_sem[ind][0]) == -1)
           perror("sem_post");

        int res_op = downloadOperation(path,tokens);

        if(sem_wait(request_sem[ind][0]) == -1)
           perror("sem_wait");

        reader_count--;

        if(reader_count == 0)
        {
            if(sem_post(request_sem[ind][1]) == -1)
                perror("sem_post");
        }

        if(sem_post(request_sem[ind][0]) == -1)
           perror("sem_post");

        if(res_op == 0)
            strcpy(res->message, "\nFile has been downloaded."); 
        else
            strcpy(res->message, "\nFile is not found."); 

     }
    }else if(strcmp("archServer", tokens[0]) == 0 && count == 2)
    {
       // archServer(tokens[1], tokens[2]); // tokens[1] server dizini, tokens[2] arşiv adı
      //  strcpy(res->message, "\nServer files are archived."); 
      printf("archServer\n");
       
    }
    else if(strcmp("quit", input) == 0)
    {
        strcpy(res->message, "\n Sending write request to server log file \n waiting for logfile ... \n logfile write request granted \n bye"); 
    }
    else if(strcmp("killServer", input) == 0)
    {
        strcpy(res->message, "\nServer is terminated, bye"); 
    }
    else
    {
        strcpy(res->message, "\nThis command is not available, type help to see available ones."); 
    }
}

void serverOp()
{
    int server_fd,  client_fd;
    create_semaphores();    

    /* create server fifo */
    umask(0);
    snprintf(server_fifo, SERVER_FIFO_LEN, SERVER_FIFO, (long) getpid());
    if(mkfifo(server_fifo,0666) == -1 && errno != EEXIST)
    {
        perror("mkfifo error");
        exit(1);
    }
    
     // open the server fifo
    server_fd = open(server_fifo, 0666);
    if(server_fd == -1)
    {
        perror("server_fd open error");
        exit(1);
    }
    
    while(exit_flag  == 0)
    {
        char client_fifo[CLIENT_FIFO_LEN];
        struct request req;
        struct response res;
        pid_t f;

        if(read(server_fd, &req, sizeof(struct request)) != sizeof(struct request))// read the request from server fifo
        {
            if(exit_flag  == 0)
            {
                perror("read request error");
                exit(1);
            }
            else
                break;
        }
    
        if(is_here(req.pid, clients, num_client) == 0)
        {
            sem_wait(client_sem);
            if(num_client < MAX_CLIENTS_NUM)
            {
                clients[num_client++] = req.pid;

              printf(">> Client PID %d connected as \"client%d\"\n", req.pid, num_client);
                   fflush(stdout);

                snprintf(client_fifo, CLIENT_FIFO_LEN, CLIENT_FIFO, (long)req.pid);
                client_fd = open(client_fifo, O_WRONLY);
                if(client_fd == -1)
                {
                    perror("client_fd open error");
                    exit(1);
                }
                strcpy(res.message, "ADD");
                if(write(client_fd, &res, sizeof(struct response)) != sizeof(struct response))
                    fprintf(stderr, "error writing to fifo %s\n", client_fifo);
                if(close(client_fd) == -1)
                {
                    perror("client_fd close error");
                    exit(1);
                }
            }
            else
            {
                printf("Connection request PID %ld . Que is full!\n", (long)req.pid);
                fflush(stdout);

                snprintf(client_fifo, CLIENT_FIFO_LEN, CLIENT_FIFO, (long) req.pid);
                client_fd = open(client_fifo, O_WRONLY);
                if(client_fd == -1)
                {
                    perror("client_fd open error");
                    exit(1);
                }
                strcpy(res.message, "FULL");
                if(write(client_fd, &res, sizeof(struct response)) != sizeof(struct response))
                    fprintf(stderr, "error writing to fifo %s\n", client_fifo);
                if(close(client_fd) == -1)
                {
                    perror("client_fd close error");
                    exit(1);
                }

                if(strcmp(req.command, "1") == 0)
                    wait_clients[waiting_num++] = req.pid;
            }
            
            sem_post(client_sem);
            continue;
        }

        /* create a child process to execute the command */
        f = fork();

        if(f > 0) 
        {
            int status;
            pid_t child_pid;

         for (;;) {
        child_pid = wait(&status);
             if (child_pid == -1 && errno == EINTR) {
             continue;
             } else {
             break;
             }
          }
        }
        else if(f == 0) 
        {

            operations(req.command, &res);
            snprintf(client_fifo, CLIENT_FIFO_LEN, CLIENT_FIFO, (long) req.pid);
            client_fd = open(client_fifo, O_WRONLY);
            if(client_fd == -1)
            {
                perror("client_fd 1open error");
                exit(1);
            }

            if(write(client_fd, &res, sizeof(struct response)) != sizeof(struct response))
                fprintf(stderr, "error writing to fifo %s\n", client_fifo);
            if(close(client_fd) == -1)
            {
                perror("client_fd close error");
                exit(1);
            }

            if(strcmp(req.command, "killServer") == 0)
            {
                kill(getppid(), SIGUSR1);
                printf("Kill signal is received.from Client PID %ld... Terminating...\n", (long)req.pid);
                printf("Bye\n");
                fflush(stdout);
                
            }
        
            exit(0);
        }
        else
        {
            perror("fork failed");
            exit(1);
        }

        /* add the command to the log file */
        sem_wait(server_log_sem);
        logFilePrint(log_file, req.pid, req.command);
        sem_post(server_log_sem);
        
        /* if the client exits, then remove it and add the next client in the waiting que */
        if(strcmp(req.command, "quit") == 0)
        {
            sem_wait(client_sem);
            /* remove the given pid from the array */
    int flagg = 0;
    for(int i = 0; i < num_client; i++)
    {
        if(clients[i] == req.pid)
        {
            flagg = 1;
            if(i != num_client-1)
               clients[i] = clients[i+1];
        }
        else if(flagg == 1 && i != num_client-1)
            clients[i] = clients[i+1];
    }
            num_client--;
            printf("Client PID %d disconnected..\n", req.pid);
            fflush(stdout);
            check_que();
            sem_post(client_sem);
        }
    }

    /* terminate all clients when server is terminated */
    for(int i = 0; i < num_client; i++)
        kill(clients[i], SIGTERM);
    for(int i = 0; i < waiting_num; i++)
    {
        kill(wait_clients[i], SIGTERM);
        char temp[50];
        sprintf(temp, "/%ld", (long) wait_clients[0]);
        sem_t* sem = sem_open(temp, 0);
        if (sem == SEM_FAILED) {
            perror("sem_open failed");
            exit(1);
        }
        if (sem_post(sem) == -1) {
            perror("sem_post failed");
            exit(1);
        }
    }

    
    /* close and unlink part */
    if(close(server_fd) == -1)
    {
        perror("server_fd close error");
        exit(1);
    }
    if(unlink(server_fifo) == -1)
    {
        perror("unlink server error");
        exit(1);
    }

    

    close_semaphores();

    exit(0);
    
}
