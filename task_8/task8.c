#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

const int tcpport = 6942;
const int udpport = 4269;

void crash(char* reason) {
    printf("crashing bc of %s\n", reason);
    exit(1);
}

struct sockaddr_in getaddr(in_addr_t saddr, int port) {
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = saddr;
    servaddr.sin_port = htons(port);
    memset(servaddr.sin_zero, 0, sizeof(servaddr.sin_zero));
    return servaddr;
}

int binded_socket(int type, in_addr_t saddr, int port) {
    struct sockaddr_in addr = getaddr(saddr, port);
    int sockfd = socket(AF_INET, type, 0);
    int bind_err = bind(sockfd, &addr, sizeof(addr));
    if(bind_err < 0) crash("bind");
    return sockfd;
}

int accept_tcp(int sockfd) {
    struct sockaddr_in cliaddr;
    unsigned int len = sizeof(cliaddr);
    int res = accept4(sockfd, &cliaddr, &len, 0);
    if(res < 0) crash("accept");
    return res;
}

int connect_tcp(in_addr_t saddr) {
    struct sockaddr_in addr = getaddr(saddr, tcpport);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int conn = connect(sockfd, &addr, sizeof(addr));
    if(conn != 0) crash("connect");
    return sockfd;
}

char* recieve_udp(int sockfd, in_addr_t* addr, int* len) {
    char* message = malloc(4096);
    struct sockaddr_in cliaddr;
    unsigned int addrlen = sizeof(cliaddr);
    int msglen = recvfrom(sockfd, message, 4096, MSG_WAITALL, &cliaddr, &addrlen);
    if(len) *len = msglen;
    if(addr) *addr = cliaddr.sin_addr.s_addr;
    return message;
}

void broadcast(in_addr_t saddr, char* message, int len) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) crash("socket");
    int flag = 1;
    int err = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag));
    if(err == -1) crash("setsockopt");
    struct sockaddr_in addr = getaddr(saddr, udpport);
    int nsend = sendto(sockfd, message, len, MSG_CONFIRM, &addr, sizeof(addr));
    if(nsend != len) crash("sendto");
    close(sockfd);
}

void server() {
    /*int socktcp = binded_socket(SOCK_STREAM, INADDR_ANY, tcpport);
    listen(socktcp, 4);
    while(1) {
        int fd = accept_tcp(socktcp);
        if(fd < 0) crash("tcp");
        printf("incoming tcp detected %d\n", fd);
        char buff[1024];
        int len = read(fd, buff, 1024);
        write(STDOUT_FILENO, buff, len);
        close(fd);
    }*/
    int sockudp = binded_socket(SOCK_DGRAM, INADDR_ANY, udpport);
    while(1) {
        int addr;
        char* buff = recieve_udp(sockudp, &addr, NULL);
        printf("incoming udp msg from %#x %s\n", addr, buff);
    }
}

void client() {
    broadcast(inet_addr("192.168.43.255"), "broadcast broadly casted", 25);
}

int main(int argc, char** argv) {
    if(argc > 1)
        if(!strcmp(argv[1], "server"))
            server();
        else if(!strcmp(argv[1], "client")) 
            client();
        else
            printf("invalid args\n");
    else
        printf("specify server or client args\n");

}
