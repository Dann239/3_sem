#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main()
{
	int sockfd;
	int n, clilen;
	char line[1024];
	struct sockaddr_un servaddr, cliaddr;
	if ((sockfd=socket(AF_UNIX, SOCK_DGRAM,0))<0) {
		perror("error on socket creation");
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));
	strcpy(servaddr.sun_path, "servaddr_sock_file");
	servaddr.sun_family = AF_UNIX;

	if (bind(sockfd, (struct sockaddr*)&servaddr,
		SUN_LEN(&servaddr)) < 0) {
		perror("binding error");
		close(sockfd);
		exit(1);
	}

	while(1) {
		clilen = sizeof(struct sockaddr_un);
		if ( n = recvfrom(sockfd, line, 1024, 0,
			(struct sockaddr*)&cliaddr,
			&clilen) < 0) {
			perror("Error on rcv");
			close(sockfd);
			exit(1);
		}
		printf("Msg received, sending back...\n");
		if ( n = sendto(sockfd, line, strlen(line), 0,
			(struct sockaddr*)&cliaddr,
			clilen) < 0) {
			perror("Error on rcv");
			close(sockfd);
			exit(1);
		}
	}
	return 0;
}