#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <stdbool.h>
#include <signal.h>

#define PATH_MAX_LENGTH 1024
#define HASH_SIZE 65536

typedef struct {
    char src_path[PATH_MAX_LENGTH];
    char dest_path[PATH_MAX_LENGTH];
} file_info_t;

typedef struct {
    file_info_t buffer[10];  // Fixed buffer size for the example
    size_t start;
    size_t end;
    int done;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} buffer_t;

buffer_t file_buffer;
pthread_t *workers;
size_t num_workers;
char *src_dir;
char *dest_dir;
unsigned long copied_files_hash[HASH_SIZE];

int regular_file_count = 0;
int fifo_file_count = 0;
int directory_count = 0;
size_t total_bytes_copied = 0;
struct timeval start_time, end_time;
size_t buffer_size;  // Global declaration

unsigned long calculate_path_hash(const char *path) {
    unsigned long hash = 5381;
    int c;
    while ((c = *path++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash % HASH_SIZE;
}

bool add_path_to_hash(const char *path) {
    unsigned long hash_index = calculate_path_hash(path);
    if (copied_files_hash[hash_index] == 0) {
        copied_files_hash[hash_index] = 1;
        return true;
    }
    return false;
}

void initialize_copied_hash() {
    for (int i = 0; i < HASH_SIZE; i++) {
        copied_files_hash[i] = 0;
    }
}

void *managerThreadStart(void *arg);
void *workerThreadStart(void *arg);
void copy_file(const char *src_path, const char *dest_path);
void copy_directory(const char *src_dir, const char *dest_dir);
void printOutputs();
void signal_handler(int signall);

void cleanup() {
    pthread_mutex_destroy(&file_buffer.mutex);
    pthread_cond_destroy(&file_buffer.not_empty);
    pthread_cond_destroy(&file_buffer.not_full);
    free(workers);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <buffer_size> <num_workers> <src_dir> <dest_dir>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    buffer_size = atoi(argv[1]);
    num_workers = atoi(argv[2]);
    src_dir = argv[3];
    dest_dir = argv[4];

    file_buffer.start = 0;
    file_buffer.end = 0;
    file_buffer.done = 0;
    pthread_mutex_init(&file_buffer.mutex, NULL);
    pthread_cond_init(&file_buffer.not_empty, NULL);
    pthread_cond_init(&file_buffer.not_full, NULL);
    workers = (pthread_t *)malloc(num_workers * sizeof(pthread_t));
    initialize_copied_hash();

    gettimeofday(&start_time, NULL);

    mkdir(dest_dir, 0755);  // Create the destination root directory here

    pthread_t manager;
    pthread_create(&manager, NULL, managerThreadStart, NULL);

    for (size_t i = 0; i < num_workers; i++) {
        pthread_create(&workers[i], NULL, workerThreadStart, NULL);
    }

    pthread_join(manager, NULL);

    for (size_t i = 0; i < num_workers; i++) {
        pthread_join(workers[i], NULL);
    }

    gettimeofday(&end_time, NULL);

    printOutputs();
    cleanup();

    return 0;
}

void signal_handler(int signall) {
    if (signall == SIGINT) {
        printf("SIGINT received. Exiting...\n");
        exit(0);
    }
}

void *managerThreadStart(void *arg) {
    copy_directory(src_dir, dest_dir);
    pthread_mutex_lock(&file_buffer.mutex);
    file_buffer.done = 1;
    pthread_cond_broadcast(&file_buffer.not_empty);
    pthread_mutex_unlock(&file_buffer.mutex);
    return NULL;
}

void *workerThreadStart(void *arg) {
    while (1) {
        pthread_mutex_lock(&file_buffer.mutex);
        while (file_buffer.start == file_buffer.end && !file_buffer.done) {
            pthread_cond_wait(&file_buffer.not_empty, &file_buffer.mutex);
        }
        if (file_buffer.start == file_buffer.end && file_buffer.done) {
            pthread_mutex_unlock(&file_buffer.mutex);
            break;
        }
        file_info_t file = file_buffer.buffer[file_buffer.start];
        file_buffer.start = (file_buffer.start + 1) % buffer_size;
        pthread_cond_signal(&file_buffer.not_full);
        pthread_mutex_unlock(&file_buffer.mutex);

        printf("Copying file from %s to -->>> %s\n", file.src_path, file.dest_path);
        copy_file(file.src_path, file.dest_path);
    }
    return NULL;
}

void copy_file(const char *src_path, const char *dest_path) {
    int src_fd = open(src_path, O_RDONLY);
    if (src_fd < 0) {
        perror("Error opening source file");
        return;
    }

    struct stat st;
    fstat(src_fd, &st);

    int dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        perror("Error opening destination file");
        close(src_fd);
        return;
    }

    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        write(dest_fd, buffer, bytes_read);
        __sync_fetch_and_add(&total_bytes_copied, bytes_read);
    }

    close(src_fd);
    close(dest_fd);

    if (S_ISREG(st.st_mode)) {
        __sync_fetch_and_add(&regular_file_count, 1);
    } else if (S_ISFIFO(st.st_mode)) {
        __sync_fetch_and_add(&fifo_file_count, 1);
    }
}

void copy_directory(const char *src_dir, const char *dest_dir) {
    DIR *dir = opendir(src_dir);
    if (!dir) {
        perror("Error opening source directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[PATH_MAX_LENGTH];
        char dest_path[PATH_MAX_LENGTH];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

        if (entry->d_type == DT_DIR) {
            mkdir(dest_path, 0755);
            __sync_fetch_and_add(&directory_count, 1);
            copy_directory(src_path, dest_path);
        } else {
            pthread_mutex_lock(&file_buffer.mutex);
            while ((file_buffer.end + 1) % buffer_size == file_buffer.start) {
                pthread_cond_wait(&file_buffer.not_full, &file_buffer.mutex);
            }
            strncpy(file_buffer.buffer[file_buffer.end].src_path, src_path, sizeof(file_buffer.buffer[file_buffer.end].src_path));
            strncpy(file_buffer.buffer[file_buffer.end].dest_path, dest_path, sizeof(file_buffer.buffer[file_buffer.end].dest_path));
            file_buffer.end = (file_buffer.end + 1) % buffer_size;
            pthread_cond_signal(&file_buffer.not_empty);
            pthread_mutex_unlock(&file_buffer.mutex);
        }
    }

    closedir(dir);
}

void printOutputs() {
    long seconds = end_time.tv_sec - start_time.tv_sec;
    long microseconds = end_time.tv_usec - start_time.tv_usec;
    long milliseconds = (seconds * 1000) + (microseconds / 1000);
    long minutes = seconds / 60;
    seconds %= 60;
    milliseconds %= 1000;

    printf("\n---------------STATISTICS--------------------\n");
    printf("Consumers: %zu - Buffer Size: %zu\n", num_workers, buffer_size);
    printf("Number of Regular File: %d\n", regular_file_count);
    printf("Number of FIFO File: %d\n", fifo_file_count);
    printf("Number of Directory: %d\n", directory_count);
    printf("TOTAL BYTES COPIED: %zu\n", total_bytes_copied);
    printf("TOTAL TIME: %02ld:%02ld.%03ld (min:sec.mili)\n", minutes, seconds, milliseconds);
}
