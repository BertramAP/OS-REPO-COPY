#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
#include<endian.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<poll.h>

#define LONESHA256_STATIC
#include "lonesha256.h"

#define THREADS 3

struct packet {
    uint8_t hash[32]; // 32 bytes
    uint64_t start; // 8 bytes
    uint64_t end; // 8 bytes
    uint8_t p; // 1 byte
}__attribute__((packed));

uint64_t reverse_sha256(uint8_t *hash, uint64_t start, uint64_t end) {
    uint8_t output[32];
    for (uint64_t i = start; i <= end; i++) {
        lonesha256(output, (const unsigned char *) &i, sizeof(i));
        if(memcmp(&output, hash, 32) == 0) {
            return i;
        }
    }
    return 0;
}



typedef struct task_t {
    int client_socket;
    struct packet packet;
} task_t; // the _t is to avoid name collision with my task pipe

typedef struct {
    task_t *data;
    int capacity;
    int count;
    int head;
    int tail;
} task_queue;

task_queue init_queue(int capacity) {
    task_queue q;
    q.data = malloc(sizeof(task_t) * capacity);
    q.capacity = capacity;
    q.count = 0;
    q.head = 0;
    q.tail = 0;
    return q;
}

//adding to the queue
void enqueue(task_queue *q, int socket, struct packet p) {
    if (q->count == q->capacity) { // check if queue is full
        int new_capacity = q->capacity * 2;
        task_t *new_data = malloc(sizeof(task_t ) * new_capacity);
        for (int i =  0; i < q->count; i++) {
            new_data[i] = q->data[(q->head + i) % q->capacity];
        }

        free(q->data);
        q->data = new_data;
        q->capacity = new_capacity;
        q->head = 0;
        q->tail = q->count;
    }

    q->data[q->tail].client_socket = socket;
    q->data[q->tail].packet = p;

    q->tail = (q->tail + 1) % q->capacity;
    q->count++;

}

// Extracting from the queue
task_t dequeue(task_queue *q) {
    if (q->count == 0) {
        fprintf(stderr, "Queue is empty\n");
        exit(EXIT_FAILURE);
    }
    task_t t = q->data[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    return t;
}

int main(int argc, char *argv[]) {
    // Server variable setup
    printf("Server starting...\n");
    int sockfd, port;
    struct sockaddr_in server_addr, cli_addr;
    struct packet packet;
    ssize_t n;

    
    if (argc > 1) {
        port = atoi(argv[1]);
    } else {
        port = 8080; // default port
    }
    printf("Packet size: %zu bytes\n", sizeof(struct packet));
    printf("Starting server on port %d...\n", port);
    // Create a TCP socket
    printf("Creating socket...\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1; 
    }
    printf("Socket created successfully.\n");
    // Set up the server address structure
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the socket to the specified port
    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }
    printf("Socket bound to port %d successfully.\n", port);
    // Listen phase
    if (listen(sockfd, 5) < 0) { // Second argument is the allowed amount of pending connections
        perror("listen");
        close(sockfd);
        return 1;
    }
    
    // Subprocesses setup, for 3 sub process and one main
    uint64_t answer; // each 4 process create their unique version of this memory anyway
    int task[THREADS][2], sol[THREADS][2];
    // Create the pipe and check for errors
    for (int i = 0; i < THREADS; i++) {
        if (pipe(task[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        if (pipe(sol[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        pid_t pid = fork();
        if (pid == 0) {
            for (int j = 0; j < i; j++) {
                close(task[j][0]);
                close(task[j][1]);
                close(sol[j][0]);
                close(sol[j][1]);
            }
            close(task[i][1]); // Close the write end of the task pipe
            close(sol[i][0]); // Close the read end of the solution pipe
            while (1) {
                while (read(task[i][0], &packet, sizeof(packet)) > 0) {
                    answer = htobe64(reverse_sha256((const unsigned char *) &packet.hash, be64toh(packet.start), be64toh(packet.end))); 
                    write(sol[i][1], &answer, sizeof(answer));
                }
            }
            exit(0);
        }
    }
    for (int i = 0; i < THREADS; i++) {
        close(task[i][0]); // Close the read end of the task pipe in the parent
        close(sol[i][1]); // Close the write end of the solution pipe in the parent
    }
    int client_sockets[THREADS];
    int busy[THREADS] = {0}; // Array to track busy subprocesses
    struct pollfd fds[THREADS + 1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
    for (int i = 0; i < THREADS; i++) {
        fds[i + 1].fd = sol[i][0]; // Monitor the read end of the solution pipe
        fds[i + 1].events = POLLIN;
    }
    task_queue queue = init_queue(100);
    socklen_t cli_len = sizeof(cli_addr);

    // Accept phase:
    while (1) {
        poll(fds, THREADS + 1, -1); // Wait indefinitely for an event
        for (int i = 0; i < THREADS; i++) { // Read the dehashed values from the subprocesses if they are ready
            if (busy[i] && (fds[i+1].revents & POLLIN)) { // If the subprocess is busy
                read(sol[i][0], &answer, sizeof(answer)); // Read the answer from the subprocess
                write(client_sockets[i], &answer, sizeof(answer)); // Send the answer back to the client
                close(client_sockets[i]); // Close the client socket
                busy[i] = 0; // Mark the subprocess as not busy
            }
        }
        if (fds[0].revents & POLLIN) { //
            int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len); // Accept a new connection
            if (newsockfd < 0) {
                perror("accept");
                continue;
            }
            //Buffer to hold the received packet
            bzero((char *) &packet, sizeof(packet));
            n = read(newsockfd, &packet, sizeof(packet));
            if (n < 0) {
                perror("read");
                close(newsockfd);
                continue;
            }
            enqueue(&queue, newsockfd, packet); // Add the new task to the queue
        }
        if (queue.count > 0) {
            for (int i = 0; i < THREADS; i++) {
                if (!busy[i] && (queue.count > 0)) { // If the subprocess is not busy
                    task_t t = dequeue(&queue); // Get the next task from the queue
                    write(task[i][1], &t.packet, sizeof(t.packet)); // Send the packet to the subprocess
                    client_sockets[i] = t.client_socket; // Store the client socket for later use
                    busy[i] = 1; // Mark the subprocess as busy
                }
            }
        }

    }
    return 0;
}
