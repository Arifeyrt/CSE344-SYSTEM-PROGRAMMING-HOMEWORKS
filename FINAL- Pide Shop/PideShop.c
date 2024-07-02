#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <complex.h>

#define MAX_COOKS 20
#define MAX_DELIVERY 20
#define MAX_ORDERS 1000
#define OVEN_CAPACITY 6
#define PREPARE_TIME 3
#define COOK_TIME 2
#define NUM_SHOVELS 3
#define MAX_BAG_CAPACITY 3

typedef struct {
    int id;
    int is_busy;
    int meals_cooked; // Aşçının pişirdiği yemek sayısı
    pthread_mutex_t lock;
} Cook;

typedef struct {
    int id;
    int random_id;
    char address[256];
    int order_id;
    int is_cancelled;
    int cook_id;
    int client_pid;
    int x, y;
    pthread_mutex_t lock;
    char status[64]; // Sipariş durumu
} Order;

typedef struct {
    int id;
    int deliveries;
    int orders_in_bag;
    int meals_delivered; // Kuryenin teslim ettiği yemek sayısı
    Order* bag[MAX_BAG_CAPACITY];
    pthread_mutex_t lock;
    pthread_mutex_t log_lock;
} DeliveryPerson;

typedef struct {
    Order orders[MAX_ORDERS];
    int count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} OrderQueue;

typedef struct {
    Order orders[MAX_ORDERS];
    int count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} ReadyQueue;

typedef struct {
    int meals_in_oven;
    pthread_mutex_t lock;
    pthread_cond_t cond_place;
    pthread_cond_t cond_remove;
    int place_door_used;
    int remove_door_used;
} Oven;

typedef struct {
    int shovels_available;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} ShovelPool;

typedef struct {
    int id;
    OrderQueue *order_queue;
    Oven *oven;
    ShovelPool *shovel_pool;
    ReadyQueue *ready_queue;
} ThreadPoolArg;

int n_cooks, m_delivery;
int delivery_speed;
Cook cooks[MAX_COOKS];
DeliveryPerson delivery_people[MAX_DELIVERY];
pthread_t cook_threads[MAX_COOKS];
pthread_t delivery_threads[MAX_DELIVERY];
OrderQueue order_queue;
ReadyQueue ready_queue;
Oven oven;
ShovelPool shovel_pool;
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;
int server_fd;
static int total_customers = 0;
static int total_served_orders = 0;
static int pid = 0;
pthread_mutex_t total_served_orders_lock = PTHREAD_MUTEX_INITIALIZER;

int calculate_travel_time(int x, int y);
void *cook_function(void *arg);
void *delivery_function(void *arg);
void *order_processing_function(void *arg);
void handle_order(int client_sock);
void log_activity(const char *activity);
void get_timestamp(char *buffer, size_t buffer_size);
void log_ready_queue();
void log_delivery(int delivery_id, Order *orders[], int count);
void setup_signal_handling();
void handle_signal(int signal);
double calculate_preparation_time();
void check_all_orders_served();

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s [portnumber] [CookthreadPoolSize] [DeliveryPoolSize] [delivery_speed]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    n_cooks = atoi(argv[2]);
    m_delivery = atoi(argv[3]);
    delivery_speed = atoi(argv[4]);

    setup_signal_handling(); // Sinyal işleyiciyi kur

    // Aşçı thread pool'u oluştur
    for (int i = 0; i < n_cooks; i++) {
        cooks[i].id = i;
        cooks[i].is_busy = 0;
        cooks[i].meals_cooked = 0; // Aşçının pişirdiği yemek sayısını başlat
        pthread_mutex_init(&cooks[i].lock, NULL);
        ThreadPoolArg *arg = malloc(sizeof(ThreadPoolArg));
        arg->id = i;
        arg->order_queue = &order_queue;
        arg->oven = &oven;
        arg->shovel_pool = &shovel_pool;
        arg->ready_queue = &ready_queue;
        pthread_create(&cook_threads[i], NULL, cook_function, arg);
    }

    // Kurye thread pool'u oluştur
    for (int i = 0; i < m_delivery; i++) {
        delivery_people[i].id = i;
        delivery_people[i].deliveries = 0;
        delivery_people[i].orders_in_bag = 0;
        delivery_people[i].meals_delivered = 0; // Kuryenin teslim ettiği yemek sayısını başlat
        pthread_mutex_init(&delivery_people[i].lock, NULL);
        pthread_mutex_init(&delivery_people[i].log_lock, NULL);
        ThreadPoolArg *arg = malloc(sizeof(ThreadPoolArg));
        arg->id = i;
        arg->order_queue = &order_queue;
        arg->oven = &oven;
        arg->shovel_pool = &shovel_pool;
        arg->ready_queue = &ready_queue;
        pthread_create(&delivery_threads[i], NULL, delivery_function, arg);
    }

    // Fırın başlangıç durumu
    oven.meals_in_oven = 0;
    pthread_mutex_init(&oven.lock, NULL);
    pthread_cond_init(&oven.cond_place, NULL);
    pthread_cond_init(&oven.cond_remove, NULL);
    oven.place_door_used = 0;
    oven.remove_door_used = 0;

    // Kürek havuzu başlangıç durumu
    shovel_pool.shovels_available = NUM_SHOVELS;
    pthread_mutex_init(&shovel_pool.lock, NULL);
    pthread_cond_init(&shovel_pool.cond, NULL);

    // ReadyQueue başlangıç durumu
    ready_queue.count = 0;
    pthread_mutex_init(&ready_queue.lock, NULL);
    pthread_cond_init(&ready_queue.cond, NULL);

    int server_fd, client_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("PideShop active, waiting for connections...\n");

    while ((client_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) >= 0) {
        // Siparişleri işlemek için yeni bir thread oluştur
        pthread_t thread_id;
        int *client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_sock;
        pthread_create(&thread_id, NULL, order_processing_function, (void *)(intptr_t)client_sock_ptr);
        pthread_detach(thread_id);  // Bağımsız thread, kendi kendine temizlenecek
    }

    if (client_sock < 0) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    close(server_fd);
    return 0;
}

void handle_signal(int signal) {
    if (signal == SIGINT) {
        printf("...detected.  Upps quiting.. writing log file\n");
        // Sipariş iptal etme işlemleri
        close(server_fd);
        exit(0);
    }
}

void setup_signal_handling() {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting up signal handler");
        exit(EXIT_FAILURE);
    }
}

void *order_processing_function(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
    handle_order(client_sock);
    close(client_sock);
    return NULL;
}

void handle_order(int client_sock) {
    char buffer[1024] = {0};
    read(client_sock, buffer, 1024);
    int client_pid, x, y, random_id;
    sscanf(buffer, "Order from client %*d at location (%d, %d) (PID %d) (RID %d)", &x, &y, &client_pid, &random_id);
    printf("%s\n", buffer);  // Siparişi ekrana yaz

    if (strncmp(buffer, "Total", 5) == 0) {
        sscanf(buffer, "Total customers: %d", &total_customers);
    }

    if (client_pid <= 0) {
        close(client_sock);
        return;
    }

    pthread_mutex_lock(&order_queue.lock);
    Order *order = &order_queue.orders[order_queue.count];
    order->id = order_queue.count + 1;
    order->random_id = random_id;
    strcpy(order->address, buffer);
    order->order_id = order->id;
    order->is_cancelled = 0;
    order->cook_id = -1;
    order->client_pid = client_pid;
    pid=client_pid;
    order->x = x;
    order->y = y;
    order_queue.count++;
    pthread_cond_signal(&order_queue.cond);
    pthread_mutex_unlock(&order_queue.lock);
}

void get_timestamp(char *buffer, size_t buffer_size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm_info = localtime(&tv.tv_sec);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
    int millis = tv.tv_usec / 1000;
    snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), ".%03d", millis);
}

// Karmaşık elemanlara sahip 30x40 matrisin psödo-tersini hesaplayan fonksiyon
double calculate_preparation_time() {
    int rows = 30;
    int cols = 40;
    double complex matrix[rows][cols];
    double complex result[rows][rows];
    
    // Rastgele karmaşık elemanlarla matrisi doldur
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double real = rand() / (double)RAND_MAX;
            double imag = rand() / (double)RAND_MAX;
            matrix[i][j] = real + imag * I;
        }
    }

    clock_t start = clock();

    // Matris çarpma işlemi
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < rows; j++) {
            result[i][j] = 0;
            for (int k = 0; k < cols; k++) {
                result[i][j] += matrix[i][k] * conj(matrix[j][k]);
            }
        }
    }

    clock_t end = clock();
    double time_taken = ((double) (end - start)) / CLOCKS_PER_SEC;

    return time_taken;
}

void *cook_function(void *arg) {
    ThreadPoolArg *pool_arg = (ThreadPoolArg *)arg;
    int cook_id = pool_arg->id;
    OrderQueue *order_queue = pool_arg->order_queue;
    Oven *oven = pool_arg->oven;
    ShovelPool *shovel_pool = pool_arg->shovel_pool;
    ReadyQueue *ready_queue = pool_arg->ready_queue;

    while (1) {
        pthread_mutex_lock(&order_queue->lock);
        while (order_queue->count == 0) {
            pthread_cond_wait(&order_queue->cond, &order_queue->lock);
        }
        Order *order = NULL;
        for (int i = 0; i < order_queue->count; i++) {
            if (!order_queue->orders[i].is_cancelled && order_queue->orders[i].cook_id == -1) {
                order = &order_queue->orders[i];
                order->cook_id = cook_id;
                break;
            }
        }
        pthread_mutex_unlock(&order_queue->lock);

        if (order != NULL) {
            char timestamp[64];
            get_timestamp(timestamp, sizeof(timestamp));
            char log_message[256];
            snprintf(log_message, sizeof(log_message), "%s - Cook %d is preparing order %d", timestamp, cook_id, order->random_id);
            log_activity(log_message);
            double preparation_time = calculate_preparation_time();
            sleep(preparation_time);

            pthread_mutex_lock(&oven->lock);
            while (oven->place_door_used) {
                pthread_cond_wait(&oven->cond_place, &oven->lock);
            }
            oven->place_door_used = 1;
            while (oven->meals_in_oven >= OVEN_CAPACITY) {
                pthread_cond_wait(&oven->cond_remove, &oven->lock);
            }
            oven->meals_in_oven++;
            get_timestamp(timestamp, sizeof(timestamp));
          //  printf("Cook %d placed order %d into the oven\n", cook_id, order->random_id);
            snprintf(log_message, sizeof(log_message), "%s - Cook %d placed order %d into the oven", timestamp, cook_id, order->random_id);
            log_activity(log_message);
            oven->place_door_used = 0;
            pthread_cond_signal(&oven->cond_place);
            pthread_mutex_unlock(&oven->lock);

            pthread_mutex_lock(&cooks[cook_id].lock);
            cooks[cook_id].is_busy = 0;
            pthread_mutex_unlock(&cooks[cook_id].lock);

            double cook_time = preparation_time / 2;
            sleep(cook_time);

            pthread_mutex_lock(&oven->lock);
            while (oven->remove_door_used) {
                pthread_cond_wait(&oven->cond_remove, &oven->lock);
            }
            oven->remove_door_used = 1;
            oven->meals_in_oven--;
            get_timestamp(timestamp, sizeof(timestamp));
        //    printf("Cook %d took out order %d from the oven\n", cook_id, order->random_id);
            snprintf(log_message, sizeof(log_message), "%s - Cook %d took out order %d from the oven", timestamp, cook_id, order->random_id);
            log_activity(log_message);
            oven->remove_door_used = 0;
            pthread_cond_signal(&oven->cond_remove);
            pthread_mutex_unlock(&oven->lock);

            pthread_mutex_lock(&shovel_pool->lock);
            shovel_pool->shovels_available++;
            pthread_cond_signal(&shovel_pool->cond);
            pthread_mutex_unlock(&shovel_pool->lock);

            get_timestamp(timestamp, sizeof(timestamp));
            snprintf(log_message, sizeof(log_message), "%s - Order %d is finished by Cook %d", timestamp, order->random_id, cook_id);
            log_activity(log_message);

            pthread_mutex_lock(&ready_queue->lock);
            ready_queue->orders[ready_queue->count] = *order;
            ready_queue->count++;
            pthread_cond_signal(&ready_queue->cond);
            pthread_mutex_unlock(&ready_queue->lock);

            pthread_mutex_lock(&order->lock);
            order->is_cancelled = 1;
            pthread_mutex_unlock(&order->lock);

            pthread_mutex_lock(&cooks[cook_id].lock);
            cooks[cook_id].meals_cooked++; // Aşçının pişirdiği yemek sayısını artır
            pthread_mutex_unlock(&cooks[cook_id].lock);

            log_ready_queue();


            pthread_mutex_lock(&oven->lock);
            if (oven->meals_in_oven < OVEN_CAPACITY) {
                pthread_cond_signal(&oven->cond_place);
            }
            pthread_mutex_unlock(&oven->lock);
        }
        sleep(1);
    }
    free(arg);
    return NULL;
}

static int my_count = 0;

void *delivery_function(void *arg) {
    ThreadPoolArg *pool_arg = (ThreadPoolArg *)arg;
    DeliveryPerson *delivery_person = &delivery_people[pool_arg->id];
    ReadyQueue *ready_queue = pool_arg->ready_queue;

    while (1) {
        int wait_time = 0;
        int max_wait_time = 15;
        while (delivery_person->orders_in_bag < MAX_BAG_CAPACITY && wait_time < max_wait_time) {
            pthread_mutex_lock(&ready_queue->lock);
            if (ready_queue->count - my_count > 0) {
                Order *order = &ready_queue->orders[my_count];
                my_count++;

                pthread_mutex_lock(&delivery_person->log_lock);
                delivery_person->bag[delivery_person->orders_in_bag++] = order;
               // printf("Delivery %d: Picked up order %d\n", delivery_person->id, order->random_id);
                strcpy(order->status, "Picked up");
                pthread_mutex_unlock(&delivery_person->log_lock);

                char timestamp[64];
                get_timestamp(timestamp, sizeof(timestamp));
                char log_message[256];
                snprintf(log_message, sizeof(log_message), "%s - Delivery %d picked up order %d", timestamp, delivery_person->id, order->random_id);
                log_activity(log_message);

                wait_time = 0;
            } else {
                pthread_mutex_unlock(&ready_queue->lock);
                sleep(1);
                wait_time++;
                continue;
            }
            pthread_mutex_unlock(&ready_queue->lock);
        }

        if (delivery_person->orders_in_bag > 0) {
            int current_x = 0;
            int current_y = 0;
            for (int i = 0; i < delivery_person->orders_in_bag; ++i) {
                Order *order = delivery_person->bag[i];

                int x_to_go = order->x - current_x;
                if (x_to_go < 0) x_to_go = -x_to_go;

                int y_to_go = order->y - current_y;
                if (y_to_go < 0) y_to_go = -y_to_go;

                current_x = order->x;
                current_y = order->y;

                pthread_mutex_lock(&delivery_person->log_lock);
              //  printf("Delivery %d: Delivering order %d to (%d,%d)\n", delivery_person->id, order->random_id, order->x, order->y);
                strcpy(order->status, "Delivering");
                pthread_mutex_unlock(&delivery_person->log_lock);

                 char timestamp[64];
                 get_timestamp(timestamp, sizeof(timestamp));
                char log_message[256];
                snprintf(log_message, sizeof(log_message), "%s - Delivery %d is delivering order %d to (%d,%d)", timestamp, delivery_person->id, order->random_id, order->x, order->y);
                log_activity(log_message);

                double road_time = (x_to_go + y_to_go) / (double)(delivery_speed * 4);
                sleep(road_time);

                pthread_mutex_lock(&delivery_person->log_lock);
             //   printf("Delivery %d: Delivered order %d\n", delivery_person->id, order->random_id);
                strcpy(order->status, "Delivered");
                delivery_person->deliveries++;
                delivery_person->meals_delivered++; // Kuryenin teslim ettiği yemek sayısını artır
                pthread_mutex_unlock(&delivery_person->log_lock);

                 get_timestamp(timestamp, sizeof(timestamp));
                snprintf(log_message, sizeof(log_message), "%s - Delivery %d delivered order %d", timestamp, delivery_person->id, order->random_id);
                log_activity(log_message);


                pthread_mutex_lock(&total_served_orders_lock);
                total_served_orders++;
                pthread_mutex_unlock(&total_served_orders_lock);
                check_all_orders_served();
            }
            delivery_person->orders_in_bag = 0;
        }
        sleep(1);
    }

    free(arg);
    return NULL;
}

void log_activity(const char *activity) {
    FILE *log_file = fopen("log.txt", "a");  // Log dosyasını aç
    if (log_file == NULL) {
        perror("Unable to open log file");
        return;
    }
    fprintf(log_file, "%s\n", activity);  // Aktiviteyi dosyaya yaz
    fclose(log_file);  // Dosyayı kapat
}

void log_ready_queue() {
    FILE *ready_file = fopen("hazirpide.txt", "w");  // Hazır siparişlerin dosyasını aç
    if (ready_file == NULL) {
        perror("Unable to open ready queue file");
        return;
    }
    pthread_mutex_lock(&ready_queue.lock);
    for (int i = 0; i < ready_queue.count; i++) {
        Order *order = &ready_queue.orders[i];
        fprintf(ready_file, "%d. hazır olan pide order %d cook %d\n", i + 1, order->random_id, order->cook_id);
    }
    pthread_mutex_unlock(&ready_queue.lock);
    fclose(ready_file);  // Dosyayı kapat
}

void log_delivery(int delivery_id, Order *orders[], int count) {
    FILE *delivery_log = fopen("deliverylog.txt", "a");
    if (delivery_log == NULL) {
        perror("Unable to open delivery log file");
        return;
    }

    fprintf(delivery_log, "Kargocu %d teslim aldı: ", delivery_id);
    for (int i = 0; i < count; i++) {
        fprintf(delivery_log, "order %d ", orders[i]->random_id);
    }
    fprintf(delivery_log, "\n");
    fclose(delivery_log);
}

void check_all_orders_served() {
    pthread_mutex_lock(&total_served_orders_lock);
    if (total_served_orders >= total_customers) {
        int max_meals_cooked = 0;
        int best_cook_id = -1;
        int max_meals_delivered = 0;
        int best_delivery_id = -1;

        for (int i = 0; i < n_cooks; i++) {
            if (cooks[i].meals_cooked > max_meals_cooked) {
                max_meals_cooked = cooks[i].meals_cooked;
                best_cook_id = i;
            }
        }

        for (int i = 0; i < m_delivery; i++) {
            if (delivery_people[i].meals_delivered > max_meals_delivered) {
                max_meals_delivered = delivery_people[i].meals_delivered;
                best_delivery_id = i;
            }
        }
        printf(">> done serving client @ %d PID %d\n", pid,pid);
        printf("Thanks Cook %d and Moto %d , cook %d cooked %d meals, moto %d delivered %d meals \n", best_cook_id, best_delivery_id,best_cook_id,cooks[best_cook_id].meals_cooked, best_delivery_id,delivery_people[best_delivery_id].meals_delivered);
        log_activity("> > active waiting for connections.");
        printf("> > active waiting for connections.\n");
        total_served_orders=0;
    }
    pthread_mutex_unlock(&total_served_orders_lock);
}