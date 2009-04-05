#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/select.h>
#include "../includes/http_server.h"

void launch_server(int port) {
	int sockfd;
	int newsockfd;
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	int n;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
	    perror("socket");
	    exit(1);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))==-1) {
	    perror("bind");
	    exit(1);
	}
	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	fcntl(sockfd, F_SETFL, O_NONBLOCK);

	fd_set rfds;
	struct timeval tv;
	int retval;

	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);


	do {
		tv.tv_sec = 60;
		tv.tv_usec = 0;
		retval = select(sockfd+1, &rfds, NULL, NULL, &tv);
		printf("TIMEOUT %d\n",retval);
	} while(retval<1);
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,&clilen);
	if (newsockfd == -1) {
	    perror("accept");
	    //exit(1);
	}

	bzero(buffer,256);
	//fcntl(newsockfd, F_SETFL, O_NONBLOCK);
	//do {
	n = read(newsockfd,buffer,255);
	write(1,buffer,n);
	if (n < 0) {
	    perror("read");
	    //exit(1);
	}
	//} while(n>0);

	n = write(newsockfd,"<html><head><title>Localhost</title></head><body>Hi you!</body></html>",70);
	if (n < 0) {
	    perror("write");
	    exit(1);
	}
	return;
}
