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
	int n, len;
	char sendline[1024], recvline[1024];
	struct sockaddr_un servaddr, cliaddr;
	if ((sockfd=socket(AF_UNIX, SOCK_DGRAM,0))<0) {
		perror("error on socket creation");
		exit(1);
	}
// filling client address structure
	bzero(&cliaddr, sizeof(cliaddr));
	cliaddr.sun_family = AF_UNIX;
	strcpy(cliaddr.sun_path,"client_sock_file");
	if (bind(sockfd, 
		(struct sockaddr*) &cliaddr, 
		SUN_LEN(&cliaddr)) < 0) {
		perror("Error on socket binding");
		close(sockfd);
		exit(1);

	}
// filling server address structure
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, "servaddr_sock_file");
	
	while(1) {
		printf("Enter a string and wait for echo:");
		fgets(sendline, 1024, stdin);
		//По-хорошему, код ниже должен быть обёрнут в циклы, но мы этого не делаем:
		//пример демонстрационный, размеры данных тренировочные, целостность и порядок
		// особо не интересуют
		if (sendto(sockfd, sendline, strlen(sendline)+1, 0,
			(struct sockaddr*)&servaddr,
			SUN_LEN(&servaddr)) < 0) {
			perror("Error on send");
			close(sockfd);
			exit(1);
		}

		if ((n = recvfrom(sockfd, recvline, 1024,0, 
		(struct sockaddr*)NULL, NULL )) < 0) {
			perror("error receiving");
			close(sockfd);
			exit(1);
		}
		recvline[n] = '\0';
		printf("%s", recvline);
	} // по-хорошему, закрытие сокета здесь нужно повесить на обработчик сигнала,
	// которым данная программа прибивается, но мне лень ;) Поэтому:
	close(sockfd); // this code is unreachable!
	return 0;
}