#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <signal.h>

void handle_signal(int signum);

typedef struct {
    char server_ip[16];
    int port;
    int client_id;
    int order_id;
    int p;
    int q;
    pid_t pid;
} client_data;

int total_customers = 0;
client_data *global_data;

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s [port] [numberOfClients] [p] [q]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = "127.0.0.1";  // Sunucu IP adresi
    int port = atoi(argv[1]);
    total_customers = atoi(argv[2]);
    int p = atoi(argv[3]);
    int q = atoi(argv[4]);

    srand(time(NULL));  // Rastgele sayı üreticisini başlat

    global_data = malloc(total_customers * sizeof(client_data));

    if (global_data == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Sunucuya toplam müşteri sayısını bildir
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        close(sock);
        return -1;
    }

    char customer_msg[256];
    snprintf(customer_msg, sizeof(customer_msg), "Total customers: %d", total_customers);
    send(sock, customer_msg, strlen(customer_msg), 0);
    close(sock);

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
      if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting up signal handler");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < total_customers; i++) {
        strncpy(global_data[i].server_ip, server_ip, 15);
        global_data[i].server_ip[15] = '\0';
        global_data[i].port = port;
        global_data[i].client_id = i;
        global_data[i].p = p;
        global_data[i].q = q;
        global_data[i].pid = getpid();
        global_data[i].order_id = rand() % 500 + 1;  // Benzersiz order_id üret

        // Thread yerine doğrudan client_function çağrısı
        client_data *data = &global_data[i];
        sock = 0;

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Socket creation error");
            continue;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(data->port);

        if (inet_pton(AF_INET, data->server_ip, &serv_addr.sin_addr) <= 0) {
            perror("Invalid address/ Address not supported");
            close(sock);
            continue;
        }

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Connection Failed");
            close(sock);
            continue;
        }

        char order[256];
        int x = rand() % data->p;
        int y = rand() % data->q;

        snprintf(order, sizeof(order), "Order from client %d at location (%d, %d) (PID %d) (RID %d)", data->client_id, x, y, data->pid, data->order_id);
        sleep(1);  // Sunucu ile arasında biraz zaman geçsin (sıralı gözüksün diye
        if (send(sock, order, strlen(order), 0) < 0) {
            perror("Send failed");
            close(sock);
            continue;
        }

        printf("Order sent: %s (PID %d)\n", order, data->pid);

        close(sock);
    }

    free(global_data);

    return 0;
}

void handle_signal(int signum) {
    for (int i = 0; i < total_customers; ++i) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            perror("Socket creation failed");
            exit(1);
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(global_data[i].port);
        if (inet_pton(AF_INET, global_data[i].server_ip, &server_addr.sin_addr) <= 0) {
            //perror("Invalid server IP");
            close(sock);
            exit(1);
        }

        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Connection to server failed");
            close(sock);
            exit(1);
        }

        char message[256];
        snprintf(message, sizeof(message), "CANCEL %d", global_data[i].order_id);
        send(sock, message, strlen(message), 0);

        close(sock);
        printf("\nOrder %d cancelled.\n", global_data[i].order_id);
    }
    exit(0);
}