#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void crash(char* reason) {
    printf("crashing bc of %s\n", reason);
    exit(1);
}

struct sockaddr_in getaddr(in_addr_t saddr, int port) {
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = saddr;
    servaddr.sin_port = htons(port);
    return servaddr;
}

int establish_socket(int type, in_addr_t saddr, int port) {
    struct sockaddr_in addr = getaddr(saddr, port);
    int sockfd = socket(AF_INET, type, 0);
    int bind_err = bind(sockfd, &addr, sizeof(addr));
    if(bind_err < 0) crash("bind");
    return sockfd;
}

char* udp_recieve(int sockfd, in_addr_t* addr) {
    char* message = malloc(4096);
    struct sockaddr_in cliaddr;
    int addrlen = sizeof(cliaddr);
    int msglen = recvfrom(sockfd, message, 4096, MSG_WAITALL, &cliaddr, &addrlen);
    if(addr) *addr = cliaddr.sin_addr.s_addr;
    printf("addr: %#x message: %s\n", cliaddr.sin_addr.s_addr, message);
    return message;
}

int main() {
    int sockfd = establish_socket(SOCK_DGRAM, INADDR_ANY, 4269);
    while(1) free(udp_recieve(sockfd, NULL));
}
