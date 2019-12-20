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
    return servaddr;
}

int establish_socket(int type, in_addr_t saddr, int port, int flags) {
    struct sockaddr_in addr = getaddr(saddr, port);
    int sockfd = socket(AF_INET | flags, type, 0);
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

int connect_tcp(in_addr_t saddr, int port, int flags) {
    struct sockaddr_in addr = getaddr(saddr, port);
    int sockfd = socket(AF_INET | flags, SOCK_STREAM, 0);
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
    printf("addr: %#x message: %s\n", cliaddr.sin_addr.s_addr, message);
    return message;
}

void server() {
    int socktcp = establish_socket(SOCK_STREAM, INADDR_ANY, tcpport, 0);
    listen(socktcp, 4);
    while(1) {
        int fd = accept_tcp(socktcp);
        if(fd < 0) crash("tcp");
        printf("incoming tcp detected %d\n", fd);
        char buff[1024];
        int len = read(fd, buff, 1024);
        write(STDOUT_FILENO, buff, len);
        close(fd);
    }
}

void client() {
    int socktcp = connect_tcp(inet_addr("127.0.0.1"), tcpport, 0);
    dprintf(socktcp, "yay tcp 4 lyfe boyessssss\n\n");
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
