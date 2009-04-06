#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>
#include "../includes/http_server.h"

void initialize_http_request_all(struct http_request client_request[MAX_CONNECTION]) {
	int i;
	for(i=0;i<MAX_CONNECTION;i++) {
		initialize_http_request(&client_request[i]);
	}
}

void initialize_http_request(struct http_request* client_request) {
	client_request->socket = -1;
	client_request->socket_length = sizeof(client_request->client_socket);
	client_request->tv.tv_sec = 0;
	client_request->tv.tv_usec = 0;
	client_request->position = 0;
}

void shut_and_close_all(struct http_request client_request[MAX_CONNECTION]) {
	int i;
	for(i=0;i<MAX_CONNECTION;i++) {
		if(client_request[i].socket!=-1) {
			shut_and_close(&client_request[i]);
		}
	}
}

void shut_and_close(struct http_request* client_request) {
	int ret;
	ret = shutdown(client_request->socket, SHUT_RDWR);
	if(ret==-1) {
		fprintf(stderr,"Failed to shutdown the connection. Trying to close...");
	}
	ret = close(client_request->socket);
	if(ret==-1) {
		fprintf(stderr,"Failed to close the connection.");
	}
	initialize_http_request(client_request);
}

int verify_data(struct http_request* client_request) {
	int position = client_request->position;
	if(position<3) {
		return 0;
	}
	if(strncmp(client_request->buffer, "GET", 3)==0) {
		printf("GET detected!\n");
		return 1;
	}
	printf("Invalid command. Closing...\n");
	return -1;
}

void set_max_nfds(int* nfds, int sockfd, struct http_request client_request[MAX_CONNECTION]) {
	*nfds = sockfd;
	int i;
	for(i=0;i<MAX_CONNECTION;i++) {
		if(client_request[i].socket>(*nfds)) {
			*nfds = client_request[i].socket;
		}
	}
}

void launch_http_server(int port) {
	int ret;

	int sockfd;
	struct http_request client_request[MAX_CONNECTION];

	initialize_http_request_all(client_request);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd==-1) {
		perror("socket");
		exit(1);
	}

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	ret = bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if(ret==-1) {
		perror("bind");
		exit(1);
	}

	ret = listen(sockfd,MAX_CONNECTION);
	if(ret==-1) {
		perror("listen");
		exit(1);
	}

	ret = fcntl(sockfd, F_SETFL, O_NONBLOCK);
	if(ret==-1) {
		perror("fcntl");
		exit(1);
	}

	fd_set globalfds;
	fd_set readfds;
	struct timeval tv;
	int nfds;

	FD_ZERO(&globalfds);
	FD_ZERO(&readfds);

	FD_SET(sockfd, &globalfds);
	nfds = sockfd;

	int i;
	for(;;) {
		do {
			printf("Waiting for activity...\n");
			readfds = globalfds;
			tv.tv_sec = SELECT_TIMEOUT;
			tv.tv_usec = 0;
			ret = select(nfds+1, &readfds, NULL, NULL, &tv);
			if(ret==-1) {
				perror("select");
				exit(1);
			}
			if(ret==0) {
				printf("Timout! Closing all sockets...\n");
				shut_and_close_all(client_request);
				set_max_nfds(&nfds, sockfd, client_request);
			}
		} while(ret<1);
		printf("Activity detected...\n");

		if(FD_ISSET(sockfd, &readfds)) {
			printf("A client is connecting...\n");
			for(i=0;i<MAX_CONNECTION;i++) {
				if(client_request[i].socket==-1) {
					break;
				}
			}
			if(i>=5) {
				fprintf(stderr,"Can't establish a new connection: the queue is full.\n");
				fprintf(stderr,"Closing...\n");
				int tmp_fd;
				struct sockaddr_in tmp_socket;
				socklen_t socket_length;
				tmp_fd = accept(sockfd, (struct sockaddr*)&tmp_socket,&socket_length);
				if(tmp_fd==-1) {
					perror("accept");
				} else {
					ret = shutdown(tmp_fd, SHUT_RDWR);
					if(ret==-1) {
						fprintf(stderr,"Failed to shutdown the connection. Trying to close...");
					}
					ret = close(tmp_fd);
					if(ret==-1) {
						fprintf(stderr,"Failed to close the connection.");
					}
				}
			} else {
				printf("Creating a new connection: #%d!\n",i);
				client_request[i].socket = accept(sockfd, (struct sockaddr*)&client_request[i].client_socket,&client_request[i].socket_length);
				if(client_request[i].socket==-1) {
					perror("accept");
				} else {
					ret = fcntl(client_request[i].socket, F_SETFL, O_NONBLOCK);
					if(ret==-1) {
						perror("fcntl");
						shut_and_close(&client_request[i]);
					} else {
						FD_SET(client_request[i].socket, &globalfds);
						if(client_request[i].socket>nfds) {
							nfds = client_request[i].socket;
						}
					}
				}
			}
		}

		int size, read_max;
		for(i=0;i<MAX_CONNECTION;i++) {
			if(client_request[i].socket==-1) {
				continue;
			}
			printf("Checking if socket #%d has something?\n",i);
			if(FD_ISSET(client_request[i].socket, &readfds)) {
				printf("Yes, reading...\n");
				read_max = BUFFER_LENGTH - 1 - client_request[i].position;
				if(read_max==0) {
					printf("The BUFFER is FULL!!!\n");
				}
				size = read(client_request[i].socket,client_request[i].buffer+client_request[i].position,read_max);
				switch(size) {
				case -1:
				perror("read");
				break;
				case 0:
					printf("Closing socket %d...\n",i);
					FD_CLR(client_request[i].socket, &globalfds);
					shut_and_close(&client_request[i]);
					set_max_nfds(&nfds, sockfd, client_request);
					break;
				default:;
				write(client_request[i].socket, client_request[i].buffer, size);
				/*
					client_request[i].position = client_request[i].position + size;
					printf("%d characters read...\n",size);
					printf("%d in total...\n",client_request[i].position);
					printf("%d remaining...\n",read_max - size);
					printf("Verifying data...\n");
					ret = verify_data(&client_request[i]);
					switch(ret) {
					case -1:
					printf("Closing socket %d...\n",i);
					FD_CLR(client_request[i].socket, &globalfds);
					shut_and_close(&client_request[i]);
					set_max_nfds(&nfds, sockfd, client_request);
					break;
					case 0:
						printf("Everything seems normal...\n");
						break;
					default:
						printf("SUCCESS!...\n");
						break;
					}
				 */
				break;
				}
			} else {
				printf("No!\n");
			}
		}
	}
	return;
}
