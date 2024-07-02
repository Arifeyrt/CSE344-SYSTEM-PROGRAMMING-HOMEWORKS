#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

void addStudentGrade(char *name, char *grade, char *filename);
void searchStudent(char *name, char *filename);
void sortAll(char *filename);
void showAll(char *filename);
void listGrades(char *filename);
void useage();
void listSome(int numofEntries, int pageNumber, char *filename);
void logging(char *message);
void gtuStudentGrades(char *filename)
{
    int fd;
    // Create child process
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Fork failed");
        logging("Fork failed\n");
    }
    else if (pid == 0)
    {
        // Create the file
        fd = creat(filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if (fd < 0)
        {
            perror("Error in creating file");
            logging("Error in creating file\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            logging("File created\n");
        }
        close(fd);
        exit(EXIT_FAILURE);
    }
    else
    {
        waitpid(pid, NULL, 0);

        // Child process terminates
    }
}

int main()
{
    char input[100];
    char command[20];
    char arg1[100];
    char arg2[100];
    char filename[100];

    while (1)
    {

        fgets(input, sizeof(input), stdin);
        if (sscanf(input, "%s", command) == 1)
        {
            if (strcmp(command, "stop") == 0)
            {
                exit(EXIT_SUCCESS); // End the loop
            }
        }

        // Call function based on user input
        if (sscanf(input, "%s", command) == 1)
        {

            if (strcmp(command, "gtuStudentGrades") == 0)
            {
                if (sscanf(input, "%*s \"%[^\"]\"", filename) == 1)
                {
                    gtuStudentGrades(filename);
                }
                else if (sscanf(input, "%*s") == 0)
                {
                    useage();
                }
                else
                {
                    printf("Invalid number of arguments for gtuStudentGrades command\n");
                    logging("Invalid number of arguments for gtuStudentGrades command\n");
                }
            }
            else if (strcmp(command, "addStudentGrade") == 0)
            {
                if (sscanf(input, "%*s \"%[^\"]\" \"%[^\"]\" \"%[^\"]\"", arg1, arg2, filename) == 3)
                {
                    addStudentGrade(arg1, arg2, filename);
                }
                else
                {
                    printf("Invalid number of arguments for addStudentGrade command\n");
                    logging("Invalid number of arguments for addStudentGrade command\n");
                }
            }
            else if (strcmp(command, "searchStudent") == 0)
            {
                if (sscanf(input, "%*s \"%[^\"]\" \"%[^\"]\"", arg1, filename) == 2)
                {
                    searchStudent(arg1, filename);
                }
                else
                {
                    printf("Invalid number of arguments for searchStudent command\n");
                    logging("Invalid number of arguments for searchStudent command\n");
                }
            }
            else if (strcmp(command, "sortAll") == 0)
            {
                if (sscanf(input, "%*s \"%[^\"]\"", filename) == 1)
                {
                    sortAll(filename);
                }
                else
                {
                    printf("Invalid number of arguments for sortAll command\n");
                    logging("Invalid number of arguments for sortAll command\n");
                }
            }
            else if (strcmp(command, "showAll") == 0)
            {
                if (sscanf(input, "%*s \"%[^\"]\"", filename) == 1)
                {
                    showAll(filename);
                }
                else
                {
                    printf("Invalid number of arguments for showAll command\n");
                    logging("Invalid number of arguments for showAll command\n");
                }
            }
            else if (strcmp(command, "listGrades") == 0)
            {
                if (sscanf(input, "%*s \"%[^\"]\"", filename) == 1)
                {
                    listGrades(filename);
                }
                else
                {
                    printf("Invalid number of arguments for lstGrades command\n");
                    logging("Invalid number of arguments for lstGrades command\n");
                }
            }
            else if (strcmp(command, "listSome") == 0)
            {
                if (sscanf(input, "%*s \"%[^\"]\" \"%[^\"]\" \"%[^\"]\"", arg1, arg2, filename) == 3)
                {
                    listSome(atoi(arg1), atoi(arg2), filename);
                }
                else
                {
                    printf("Invalid number of arguments for lstSome command\n");
                    logging("Invalid number of arguments for lstSome command\n");
                }
            }
            else
            {
                printf("Invalid command\n");
                logging("Invalid command\n");
            }
        }
        else
        {
            printf("Invalid command\n");
            logging("Invalid command\n");
        } 
    }

    return 0;
}

// Add student grade and name
void addStudentGrade(char *name, char *grade, char *filename)
{

    pid_t pid = fork();
    char message[200];

    if (pid < 0)
    {
        perror("Fork failed");
        logging("Fork failed\n");
    }
    else if (pid == 0)
    {
        int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

        if (fd < 0)
        {
            perror("Error opening file");
            logging("Error opening file\n");
            exit(EXIT_FAILURE);
        }
        char buffer[500]; // Will hold the data to be written
        sprintf(buffer, "%s,%s\n", name, grade);
        if (write(fd, buffer, strlen(buffer)) < 0)
        {
            perror("Error writing to file");
            logging("Error writing to file\n");
            exit(EXIT_FAILURE);
        }
        strcat(message, name);
        strcat(message, " ");
        strcat(message, grade);
        strcat(message, " ");
        strcat(message, filename);
        strcat(message, " Student grade added\n");
        logging(message);

        close(fd);
        exit(EXIT_SUCCESS);
    }
    else
    {
        waitpid(pid, NULL, 0);
    }
}

// Student search function
void searchStudent(char *name, char *filename)
{
    char message[200];
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Fork failed");
        logging("Fork failed\n");
    }
    else if (pid == 0)
    {
        int fd = open(filename, O_RDONLY); // I opened the file in read mode
        if (fd < 0)
        {
            perror("Error opening file");
            logging("Error opening file\n");
            exit(EXIT_FAILURE);
        }

        char line[500];   //I created a array to hold the line
        int bytesRead;    //Keeps the number of bytes read
        char *nameToken;  // Student name
        char *gradeToken; // Student grade

        int i = 0;
        while ((bytesRead = read(fd, line, sizeof(line))) == -1 && (errno == EINTR))
            ;
        while (i < bytesRead)
        {
            int newlineIndex = i;
            while (newlineIndex < bytesRead && line[newlineIndex] != '\n')
            {
                newlineIndex++;
            }

           // Copies the line to the buffer and adds a null character
            line[newlineIndex] = '\0';

            // Breaks the line according to comma character and assigns it to name and grade variables
            nameToken = strtok(line + i, ",");
            gradeToken = strtok(NULL, ",");

           // If the second piece of fragmented data is not null (i.e. there is a grade)
            if (gradeToken != NULL)
            {
                // If student name is equal to the searched student name
                if (strcmp(nameToken, name) == 0)
                {
                    printf("%s,%s\n", nameToken, gradeToken); 
                    strcat(message, name);
                    strcat(message, " ");
                    strcat(message, filename);
                    strcat(message, " Student found\n");
                    logging(message);
                    close(fd);
                    exit(EXIT_SUCCESS);
                    return;
                }
            }

            // Move to next line
            i = newlineIndex + 1;
        }

        strcat(message, name);
        strcat(message, " ");
        strcat(message, filename);
        strcat(message, " Student not found\n");
        printf("Student not found.\n"); // If the student is not found, write to the screen
        logging(message);
        close(fd);
        exit(EXIT_FAILURE);
    }
    else
    {
        waitpid(pid, NULL, 0);
    }
}

typedef struct
{
    char name[50];
    char grade[50];
} Student;

// Comparison function
int compare(const void *a, const void *b)
{
    return strcmp(((Student *)a)->name, ((Student *)b)->name);
}

// Function of reading names and notes from the file and printing them on the screen in alphabetical order
void sortAll(char *filename)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Fork failed");
        logging("Fork failed\n");
    }

    else if (pid == 0)
    {
        int fd = open(filename, O_RDONLY); 
        if (fd < 0)
        {
            perror("Error opening file");
            logging("Error opening file\n");
            exit(EXIT_FAILURE);
        }

        Student records[500]; // Will keep 500 student information
        char line[500];      // A array to hold the line
        int bytesRead;      // Keeps the number of bytes read
        char *nameToken;      //Holds the student name
        char *gradeToken;     // Student takes notes
        int recordCount = 0;  // Keeps the number of students read
        int i = 0;

        while ((bytesRead = read(fd, line, 500)) == -1 && (errno == EINTR))
            ;

        while (i < bytesRead)
        {
            int newlineIndex = i;
            while (newlineIndex < bytesRead && line[newlineIndex] != '\n')
            {
                newlineIndex++;
            }

            //Copy line and add null character
            line[newlineIndex] = '\0';

            // Breaks the line according to comma character
            nameToken = strtok(line + i, ",");
            gradeToken = strtok(NULL, ",");

            if (gradeToken != NULL)
            {
               // Add student name and grade to array
                strcpy(records[recordCount].name, nameToken);
                strcpy(records[recordCount].grade, gradeToken);
                recordCount++;
            }

        // Move to next line
            i = newlineIndex + 1;
        }

       // Sort the array
        qsort(records, recordCount, sizeof(Student), compare);

      // Write the sorted array to the screen
        for (int i = 0; i < recordCount; i++)
        {
            printf("%s,%s\n", records[i].name, records[i].grade);
        }
        logging("All students sorted\n");

        close(fd);
        exit(EXIT_SUCCESS);
    }
    else
    {
        waitpid(pid, NULL, 0);
    }
}

// Showing all students
void showAll(char *filename)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Fork failed");
        logging("Fork failed\n");
    }
    else if (pid == 0)
    {
        int fd = open(filename, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if (fd < 0)
        {
            perror("Errorrr opening file");
            logging("Error opening file\n");
            exit(EXIT_FAILURE);
        }

        char buffer[500];
        int bytesRead;
        // Dosyadan veri oku
        while ((bytesRead = read(fd, buffer, sizeof(buffer))) == -1 && (errno == EINTR))
            ;

        write(STDOUT_FILENO, buffer, bytesRead); // Write the read data to the screen
        logging("All students shown\n");
        // Check if there is a read error
        if (bytesRead < 0)
        {
            perror("Error reading file");
            logging("Error reading file\n");
            exit(EXIT_FAILURE);
        }

        close(fd);
        exit(EXIT_SUCCESS);
    }
    else
    {
        waitpid(pid, NULL, 0);
    }
}

// Listing notes
void listGrades(char *filename)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Fork failed");
        logging("Fork failed\n");
    }
    else if (pid == 0)
    {
        int fd = open(filename, O_RDONLY); //Open the file in read mode
        if (fd == -1)
        {
            perror("Errorrrr opening file");
            logging("Error opening file\n");
            exit(EXIT_FAILURE);
        }

        char buffer[500];
        ssize_t bytesRead;
        int count = 0; // Counter to count the first 5 students
        int firstfive = 0;

        // Reading data from the file and printing it to the screen
        while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0 && count < 5 && (errno != EINTR))
        {
            for (ssize_t i = 0; i < bytesRead; i++)
            {
                if (buffer[i] == '\n')
                { //Increment counter based on end of line character
                    firstfive = i;
                    count++;
                    if (count >= 5)
                    {
                        break; //End the loop after printing the first 5 student information
                    }
                }
            }
            if (count < 5)
            {
                ssize_t bytesWritten = write(STDOUT_FILENO, buffer, bytesRead); // Write the read data to the screen
            }
            else
            {
                ssize_t bytesWritten = write(STDOUT_FILENO, buffer, firstfive + 1);
                logging("First 5 students listed\n");
            }
        }

        //Check for read errors
        if (bytesRead < 0)
        {
            perror("Error reading file");
            logging("Error reading file\n");
            exit(EXIT_FAILURE);
        }

        close(fd);
        exit(EXIT_SUCCESS);
    }
    else
    {
        waitpid(pid, NULL, 0);
    }
}

// Showing a certain number of students
void listSome(int numofEntries, int pageNumber, char *filename)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Fork failed");
        logging("Fork failed\n");
    }
    else if (pid == 0)
    {
        int fd = open(filename, O_RDONLY);
        if (fd == -1)
        {
            perror("Error opening file");
            logging("Error opening file\n");
            exit(EXIT_FAILURE);
        }

        char buffer[200];
        ssize_t bytesRead;
        int lineCount = 0;                               // Counter to count lines read from file
        int startLine = (pageNumber - 1) * numofEntries;// Calculates the starting row
        int endLine = pageNumber * numofEntries;         //Calculate the ending row
        int i = 0;
        while ((bytesRead = read(fd, buffer, sizeof(buffer))) == -1 && (errno == EINTR))
            ;
       // Read data from file
        for (ssize_t j = 0; j < bytesRead; j++)
        {
            if (buffer[j] == '\n')
            { 
                lineCount++;
                if (lineCount > startLine && lineCount <= endLine)
                {// If the number of lines read is between the start and end lines, write the read data to the screen
                    write(STDOUT_FILENO, buffer + i, j - i + 1);
                }
                i = j + 1;
            }
        }

        // Check for read errors
        if (bytesRead < 0)
        {
            perror("Error reading file");
            logging("Error reading file\n");
            exit(EXIT_FAILURE);
        }
        logging("Some students listed\n");
     
        close(fd);
        exit(EXIT_SUCCESS);
    }
    else
    {
        waitpid(pid, NULL, 0);
    }
}

void useage()
{

    printf("to add to the file: addStudentGrade \"name surname\" \"grade\" \"filename\"\n");
    printf("to search for students: searchStudent \"name surname\" \"filename\"\n");
    printf("to see all students name in alphabetical order: sortAll \"filename\"\n");
    printf("to see all students: showAll \"filename\"\n");
    printf("to list the top five students: lstGrades \"filename\"\n");
    printf("to list a specific number of students: lstSome \"numofEntries\" \"pageNumber\" \"filename\"\n");
    printf("to create file: gtuStudentGrades \"filename\"\n");
    logging("exit\n");
}

void logging(char *message)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Fork failed");
    }
    else if (pid == 0)
    {
        int fd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

        if (fd < 0)
        {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }
        if (write(fd, message, strlen(message)) < 0)
        {
            perror("Error writing to file");
            exit(EXIT_FAILURE);
        }
        close(fd);
        exit(EXIT_SUCCESS);
    }
    else
    {
        waitpid(pid, NULL, 0);
    }
}
