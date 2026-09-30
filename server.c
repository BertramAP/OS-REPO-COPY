#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
#include<endian.h>
#include<sys/socket.h>

struct sockaddr {
   unsigned short   sa_family;
   char             sa_data[14];
};

struct packet {
    uint8_t hash[32]; // 32 bytes
    uint64_t start; // 8 bytes
    uint64_t end; // 8 bytes
    uint8_t p; // 1 byte
}__attribute__((packed));

struct in_addr {
   unsigned long s_addr;
};

struct sockaddr_in {
   short int            sin_family;
   unsigned short int   sin_port;
   struct in_addr       sin_addr;
   unsigned char        sin_zero[8];
};



int main(int argc, char *argv[]) {
    int sockfd, port;
    if (argc > 0) {
        port = atoi(argv[1]);
    } else {
        port = 8080; // default port
    }
    printf("Packet size: %zu bytes\n", sizeof(struct packet));
    printf("Starting server on port %d...\n", port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0) {
        perror("socket");
        close(sockfd);
        return 1;
    }
        
    return 0;
}
