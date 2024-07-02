#include "Fifostruct.h"
#include <libgen.h>

int isValidNumber(char const str[])
{
    int i = 0;
    while (str[i] != '\0')
    {
        if (isdigit(str[i]) == 0)
            return 0;
        i++;
    }
    return 1;
}
/* list command */
void listOperation(char output[])
{
    FILE *fp = popen("ls", "r");

    if (fp == NULL)
    {
        perror("Failed to run command\n");
        exit(1);
    }
    char buffer[MESSAGE_LEN] = "";
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        strcat(output, buffer);
    }

    if (pclose(fp) == -1)
    {
        perror("Error closing the pipe");
        exit(1);
    }
}

/* readF command */
void readEntireFile(char tokens[4][50], char resp[])
{
    FILE *fp = fopen(tokens[1], "r");
    if (fp == NULL)
    {
        strcpy(resp, "File not found");
        return;
    }

    char buffer[MESSAGE_LEN];
    strcpy(resp, "");

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        strcat(resp, buffer);
    }

    if (fclose(fp) == -1)
    {
        perror("Error close");
        exit(1);
    }
}
void readSpecificLine(char tokens[4][50], char resp[])
{
    printf("tokken 1: %s\n", tokens[1]);
    FILE *fp = fopen(tokens[1], "r");
    if (fp == NULL)
    {
        strcpy(resp, "File not found");
        return;
    }

    int line_num;
    if (isValidNumber(tokens[2]) == 1)
        line_num = atoi(tokens[2]);
    else
    {
        strcpy(resp, "Invalid line number");
        return;
    }
    int current_line = 1;
    char buffer[MESSAGE_LEN];
    strcpy(resp, "");

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        printf("buffer: %s\n", buffer);
        if (current_line == line_num)
        {
            strcpy(resp, buffer);
            break;
        }
        current_line++;
    }
    if (fclose(fp) == -1)
    {
        perror("Error close");
        exit(1);
    }
}
void writeTWithLine(char tokens[4][50], char resp[])
{
    char temp_file_name[] = "temp.txt";
    char line[500];
    int line_num;
    int current_line_num = 0;

    FILE *input_file = fopen(tokens[1], "r");
    FILE *temp_file = fopen(temp_file_name, "w");
    if (input_file == NULL)
    {
        input_file = fopen(tokens[1], "w+");
        if (input_file == NULL)
        {
            perror("Failed to open file");
            exit(1);
        }
    }
    if (temp_file == NULL)
    {
        perror("Failed to open file");
        exit(1);
    }

    if (isValidNumber(tokens[2]) == 1)
        line_num = atoi(tokens[2]);
    else
    {
        strcpy(resp, "\nInvalid line number");
        return;
    }

    while (fgets(line, 500, input_file))
    {
        current_line_num++;
        if (current_line_num == line_num)
        {
            fputs(tokens[3], temp_file);
            fputs("\n", temp_file);
        }
        fputs(line, temp_file);
    }
    if (fclose(input_file) == -1)
    {
        perror("Error closing the pipe");
        exit(1);
    }
    if (fclose(temp_file) == -1)
    {
        perror("Error closing the pipe");
        exit(1);
    }

    // Dosyanın eski versiyonunu sil ve temp dosyasını yeniden adlandır
    if (remove(tokens[1]) != 0 || rename(temp_file_name, tokens[1]) != 0)
    {
        perror("Error renaming file");
        exit(1);
    }

    strcpy(resp, "\nThe given string is written to the file");
}
void writeTWithoutLine(char tokens[4][50], char resp[])
{
    char temp_file_name[] = "temp.txt";
    FILE *input_file = fopen(tokens[1], "r");
    FILE *temp_file = fopen(temp_file_name, "w");
    int new_flag = 0;

    if (input_file == NULL)
    {
        new_flag = 1;
        input_file = fopen(tokens[1], "w+");
        if (input_file == NULL)
        {
            perror("Failed to open file");
            exit(1);
        }
    }
    if (temp_file == NULL)
    {
        perror("Failed to open file");
        exit(1);
    }
    char line[500];
    int current_line_num = 0;

    while (fgets(line, 500, input_file))
    {
        current_line_num++;
        fputs(line, temp_file);
    }

    if (new_flag == 0)
        fputs("\n", temp_file);

    fputs(tokens[2], temp_file);

    if (fclose(input_file) == -1)
    {
        perror("Error closing the pipe");
        exit(1);
    }
    if (fclose(temp_file) == -1)
    {
        perror("Error closing the pipe");
        exit(1);
    }

    if (remove(tokens[1]) != 0)
    {
        perror("Error removing file");
        exit(1);
    }
    if (rename(temp_file_name, tokens[1]) != 0)
    {
        perror("Error renaming file");
        exit(1);
    }

    strcpy(resp, "\nThe given string is written to the file");
}

/* upload command */
int copy_file(const char *source, const char *destination) {
    FILE *source_file = fopen(source, "r");
    if (source_file == NULL) {
        perror("Error opening source file");
        return -1;
    }

    FILE *dest_file = fopen(destination, "w");
    if (dest_file == NULL) {
        perror("Error opening destination file");
        fclose(source_file);
        return -1;
    }

    int ch;
    while ((ch = fgetc(source_file)) != EOF) {
        if (fputc(ch, dest_file) == EOF) {
            perror("Error writing to destination file");
            fclose(source_file);
            fclose(dest_file);
            return -1;
        }
    }

    fclose(source_file);
    fclose(dest_file);
    return 0;
}

int uploadOperation(char path[50], char tokens[4][50]) {
    // Dosyanın bulunduğu dizinin adını al
    char directory[100];
    strcpy(directory, path);
    char *dir_name = dirname(directory);

    // Dosyanın bulunduğu dizinin yolu
    char file_path[100];
    sprintf(file_path, "../%s", path);

    // Dosyanın hedef dizine kopyalanacağı yolu oluşturma
    char dest_path[100];
    sprintf(dest_path, "./%s", tokens[1]);

    // Dosyanın varlığını kontrol etme
    if (access(file_path, F_OK) != 0)
        return -1;

    // Dosyayı kopyalama işlemi
    if (copy_file(file_path, dest_path) != 0)
        return -1;

    // Dosyayı başarıyla taşıdık, şimdi eski konumundaki dosyayı silelim
    char old_file_path[100];
    sprintf(old_file_path, "../%s/%s", dir_name, tokens[1]);
    if (remove(old_file_path) != 0) {
        perror("Error deleting old file");
        return -1;
    }

    return 0;
}

int downloadOperation( char path[50],char tokens[4][50])
{
    FILE *stream;
    char full_command[100];

    if (access(tokens[1], F_OK) != 0)
        return -1;

    char new_path[50] = "";
    char *token = strtok(path, "/");
    while (token != NULL)
    {
        strcat(new_path, "../");
        token = strtok(NULL, "/");
    }

    sprintf(full_command, "cp %s %s", tokens[1], new_path);
    stream = popen(full_command, "r");
    if (stream == NULL)
    {
        perror("Error executing command");
        exit(1);
    }
    if (pclose(stream) == -1)
    {
        perror("Error closing the pipe");
        exit(1);
    }
        // Dosyayı başarıyla taşıdık, şimdi eski konumundaki dosyayı silelim
    if (remove(tokens[1]) != 0) {
        perror("Error deleting old file");
        return -1;
    }

    return 0;
}
/*
void archServer(const char *sv_dir, const char *archive_name) {
    // Create temporary directory path
    char temp_dir_path[PATH_MAX];
    snprintf(temp_dir_path, sizeof(temp_dir_path), "./%s_temp", archive_name);

    // Create temporary directory
    if (mkdir(temp_dir_path, 0777) == -1 && errno != EEXIST) {
        perror("Failed to create temporary directory");
        return;
    }

    // Copy files from server directory to temporary directory
    DIR *dir = opendir(sv_dir);
    if (dir == NULL) {
        perror("Failed to open server directory");
        return;
    }

    struct dirent *entry;
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {  // Regular file
            snprintf(src_path, sizeof(src_path), "%s/%s", sv_dir, entry->d_name);
            snprintf(dest_path, sizeof(dest_path), "%s/%s", temp_dir_path, entry->d_name);
            if (link(src_path, dest_path) == -1) {
                perror("Failed to link files");
                return;
            }
        }
    }
    closedir(dir);

    // Create archive file path
    char archive_path[PATH_MAX];
    snprintf(archive_path, sizeof(archive_path), "%s/%s.tar.gz", sv_dir, archive_name);

    // Fork a child process to create the archive
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Child process: create the archive
        execlp("tar", "tar", "-czf", archive_path, "-C", temp_dir_path, ".", NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process: wait for the child to finish
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("Archive created successfully.\n");

            // Remove temporary directory
            DIR *temp_dir = opendir(temp_dir_path);
            if (temp_dir) {
                while ((entry = readdir(temp_dir)) != NULL) {
                    snprintf(dest_path, sizeof(dest_path), "%s/%s", temp_dir_path, entry->d_name);
                    unlink(dest_path); // Remove files inside the temporary directory
                }
                closedir(temp_dir);
                if (rmdir(temp_dir_path) != 0) {
                    perror("Failed to remove temporary directory");
                    return;
                }
            }
        } else {
            printf("Failed to create archive.\n");
        }
    }
}
*/
/* help command */
void help_op(char command[50], char resp[])
{
    if (strcmp(command, "list") == 0)
    {
        strcpy(resp, "list\n display the list of possible client requests\n sends a request to display the list of files in Servers directory\n (also displays the list received from the Server)\n");
    }
    else if (strcmp(command, "readF") == 0)
    {
        strcpy(resp, "readF <file> <line #>\n requests to display the # line of the <file>, if no line number is given the whole contents of the file is requested (and displayed on the client side)\n");
    }
    else if (strcmp(command, "writeT") == 0)
    {
        strcpy(resp, "writeT <file> <line #> <string>\n request to write the content of “string” to the #th line the <file>, if the line # is not given writes to the end of file. If the file does not exists in Servers directory creates and edits the file at the same time\n");
    }
    else if (strcmp(command, "upload") == 0)
    {
        strcpy(resp, "upload <file>\n uploads the file from the current working directory of client to the Servers directory\n (beware of the cases no file in clients current working directory and file with the same name on Servers side)\n");
    }
    else if (strcmp(command, "download") == 0)
    {
        strcpy(resp, "download <file>\n request to receive <file> from Servers directory to client side\n");
    }
    else if (strcmp(command, "archServer") == 0)
    {
        strcpy(resp, "archServer <fileName>.tar\n Using fork, exec and tar utilities create a child process that will collect all the files currently available on the the Server side and store them in the <filename>.tar archive\n");
    }
    else if (strcmp(command, "killServer") == 0)
    {
        strcpy(resp, "killServer\n Sends a kill request to the Server\n");
    }
    else if (strcmp(command, "quit") == 0)
    {
        strcpy(resp, "quit\n Send write request to Server side log file and quits\n");
    }
    else
    {
        strcpy(resp, "invalid command\n");
    }
}

void logFilePrint(char filename[], pid_t pid, char command[])
{
    /* Log dosyasını aç */
    FILE *file = fopen(filename, "a");
    if (file == NULL)
    {
        perror("Not open log file.");
        exit(1);
    }

    /* İstemci PID'sini yaz */
    fprintf(file, "Client PID: %d\n", pid);

    /* Komutu yaz */
    fprintf(file, "command: %s\n", command);

    /* Log dosyasını kapat */
    if (fclose(file) == EOF)
    {
        perror("error:close log file");
        exit(1);
    }
}