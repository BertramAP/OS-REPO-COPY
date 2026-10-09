#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
#include<endian.h>
#include <netinet/in.h>
#include<sys/socket.h>

#define LONESHA256_STATIC
#include "lonesha256.h"

#define THREADS 4

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
    pid_t pid1, pid2, pid3;
    uint64_t answer; // each 4 process create their unique version of this memory anyway
    int fd[2]; 


    // Accept phase:
    while (1) {
        // Create the pipe and check for errors

        socklen_t cli_len = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);
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
        // Print the received packet details for debug
        /*
        printf("Received packet:\n");
        printf("Hash: ");
        for (int i = 0; i < 32; i++) {
            printf("%02x", packet.hash[i]);
        }
        printf("\nStart: %lu\n", be64toh(packet.start));
        printf("End: %lu\n", be64toh(packet.end));
        printf("P: %u\n", packet.p);
        */
        // Delegate the hashes to the subprocesses
        answer = htobe64(reverse_sha256((const unsigned char *) &packet.hash, be64toh(packet.start), be64toh(packet.end)));
        n = write(newsockfd,(const unsigned char *) &answer, 8);
        if (n < 0) {
            perror("write");
            close(newsockfd);
            continue;
        }
        
        close(newsockfd);
        close(fd[0]);
    }
    return 0;
}
