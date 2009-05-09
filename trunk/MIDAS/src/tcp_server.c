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
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../includes/tcp.h"
#include "../includes/tcp_server.h"
#include "../includes/log.h"

int initialize_tcp_request_all(struct _tcp_session client_request[MAX_CONNECTION]) {
	int i;
	int ret;
	for(i=0;i<MAX_CONNECTION;i++) {
		ret = initialize_tcp_request(&client_request[i]);
		if(ret==0) {
			log_message_error("Initializing TCP sockets",PROGRAM,"Attempt failed");
			return 0;
		}
	}
	return 1;
}

int shut_and_close_all(struct _tcp_session client_request[MAX_CONNECTION]) {
	int i;
	int ret;
	char concat[1024];
	for(i=0;i<MAX_CONNECTION;i++) {
		if(client_request[i].socket!=-1) {
			ret = shut_and_close(&client_request[i]);
			if(ret==0) {
				log_message_error("Closing TCP sockets",PROGRAM,"Attempt failed");
				return 0;
			}
			snprintf(concat,1024,"Socket %d (%s) is now closed",i,inet_ntoa(client_request[i].client_socket.sin_addr));
			log_message("Closing TCP client socket",concat);
		}
	}
	return 1;
}

void set_max_nfds(int* nfds, int sockfd, struct _tcp_session client_request[MAX_CONNECTION]) {
	*nfds = sockfd;
	int i;
	for(i=0;i<MAX_CONNECTION;i++) {
		if(client_request[i].socket>(*nfds)) {
			*nfds = client_request[i].socket;
		}
	}
}

void set_protocol_initializer(struct _tcp_session client_request[MAX_CONNECTION], void (*protocol_initializer)(struct _tcp_session*)) {
	int i;
	for(i=0;i<MAX_CONNECTION;i++) {
		protocol_initializer(&client_request[i]);
	}
}

void launch_tcp_server(int port, void (*protocol_initializer)(struct _tcp_session*)) {
	int ret;
	int sel;
	char concat[1024];

	int sockfd;
	struct _tcp_session client_request[MAX_CONNECTION];

	set_protocol_initializer(client_request, protocol_initializer);
	ret = initialize_tcp_request_all(client_request);
	if(ret==0) {
		return;
	}

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sockfd==-1) {
		log_message_error("Creating TCP socket",SYSTEM,"socket");
		return;
	}

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = PF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	ret = bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if(ret==-1) {
		log_message_error("Binding TCP socket",SYSTEM,"bind");
		return;
	}

	ret = listen(sockfd,MAX_CONNECTION);
	if(ret==-1) {
		log_message_error("Listening TCP socket",SYSTEM,"listen");
		return;
	}

	ret = fcntl(sockfd, F_SETFL, O_NONBLOCK);
	if(ret==-1) {
		log_message_error("Setting non-blocking TCP socket",SYSTEM,"fcntl");
		return;
	}

	fd_set globalrfds;
	fd_set readfds;
	fd_set globalwfds;
	fd_set writefds;
	struct timeval tv;
	int nfds;

	FD_ZERO(&globalrfds);
	FD_ZERO(&readfds);
	FD_ZERO(&globalwfds);
	FD_ZERO(&writefds);

	FD_SET(sockfd, &globalrfds);
	nfds = sockfd;

	log_message("TCP Server","Successfully started");

	int i;
	for(;;) {
		do {
			readfds = globalrfds;
			writefds = globalwfds;
			tv.tv_sec = SELECT_TIMEOUT;
			tv.tv_usec = 0;
			sel = select(nfds+1, &readfds, &writefds, NULL, &tv);
			if(sel==-1) {
				log_message_error("Listening HTTP socket",SYSTEM,"select");
				return;
			}
			if(sel==0) {
				ret = shut_and_close_all(client_request);
				if(ret==0) {
					return;
				}
				set_max_nfds(&nfds, sockfd, client_request);
			}
		} while(sel<1);

		if(FD_ISSET(sockfd, &readfds)) {
			for(i=0;i<MAX_CONNECTION;i++) {
				if(client_request[i].socket==-1) {
					break;
				}
			}
			if(i>=MAX_CONNECTION) {
				log_message_error("Connecting new client",PROGRAM,"Impossible. No socket left");
				int tmp_fd;
				struct sockaddr_in tmp_socket;
				socklen_t socket_length;
				tmp_fd = accept(sockfd, (struct sockaddr*)&tmp_socket,&socket_length);
				if(tmp_fd==-1) {
					log_message_error("Accepting new client",SYSTEM,"accept");
				} else {
					ret = shutdown(tmp_fd, SHUT_RDWR);
					if(ret==-1) {
						log_message_error("Shutting down new client socket",SYSTEM,"shutdown");
						log_message_error("Shutting down new client socket",WARNING,"Attempt failed, trying to close");
					}
					ret = close(tmp_fd);
					if(ret==-1) {
						log_message_error("Closing new client socket",SYSTEM,"close");
						log_message_error("Closing new client socket",WARNING,"Attempt failed");
					}
				}
			} else {
				client_request[i].socket = accept(sockfd, (struct sockaddr*)&client_request[i].client_socket,&client_request[i].socket_length);
				if(client_request[i].socket==-1) {
					log_message_error("Accepting new client",SYSTEM,"accept");
				} else {
					ret = fcntl(client_request[i].socket, F_SETFL, O_NONBLOCK);
					if(ret==-1) {
						log_message_error("Setting non-blocking TCP socket",SYSTEM,"fcntl");
						ret = shut_and_close(&client_request[i]);
						if(ret==0) {
							return;
						}
					} else {
						snprintf(concat,1024,"%s is now connected on socket %d, port %d",inet_ntoa(client_request[i].client_socket.sin_addr),i,port);
						log_message("New TCP client",concat);
						FD_SET(client_request[i].socket, &globalrfds);
						if(client_request[i].socket>nfds) {
							nfds = client_request[i].socket;
						}
					}
				}
			}
		}

		int size, read_max, write_max;
		for(i=0;i<MAX_CONNECTION;i++) {
			if(client_request[i].socket==-1) {
				continue;
			}
			if(FD_ISSET(client_request[i].socket, &readfds)) {
				read_max = BUFFER_LENGTH - client_request[i].position;
				if(read_max==0) {
					log_message_error("Reading TCP socket",PROGRAM,"Buffer overflow, aborting");
					size = 0;
				} else {
					size = read(client_request[i].socket,client_request[i].buffer+client_request[i].position,read_max);
				}
				switch(size) {
				case -1:
				log_message_error("Reading TCP socket",SYSTEM,"read");
				break;
				case 0:
					FD_CLR(client_request[i].socket, &globalrfds);
					ret = shut_and_close(&client_request[i]);
					if(ret==0) {
						return;
					}
					set_max_nfds(&nfds, sockfd, client_request);
					break;
				default:
					client_request[i].position = client_request[i].position + size;
					ret = client_request[i].protocol_reader(&client_request[i]);
					switch(ret) {
					case -1:
					FD_CLR(client_request[i].socket, &globalrfds);
					ret = shut_and_close(&client_request[i]);
					if(ret==0) {
						return;
					}
					set_max_nfds(&nfds, sockfd, client_request);
					break;
					case 0: /* Everything is normal, continuing... */
						break;
					case 1:
						client_request[i].position = 0;
						FD_SET(client_request[i].socket, &globalwfds);
						FD_CLR(client_request[i].socket, &globalrfds);
						break;
					}
					break;
				}
			}
		}
		for(i=0;i<MAX_CONNECTION;i++) {
			if(client_request[i].socket==-1) {
				continue;
			}
			if(FD_ISSET(client_request[i].socket, &writefds)) {
				if(client_request[i].position==0) {
					ret = client_request[i].protocol_writer(&client_request[i]);
					if(ret==0) {
						FD_CLR(client_request[i].socket, &globalwfds);
						ret = shut_and_close(&client_request[i]);
						if(ret==0) {
							return;
						}
						snprintf(concat,1024,"Socket %d (%s), port %d, is now closed",i,inet_ntoa(client_request[i].client_socket.sin_addr),port);
						log_message("Closing TCP client socket",concat);
						set_max_nfds(&nfds, sockfd, client_request);
						continue;
					}
				}
				write_max = client_request[i].limit - client_request[i].position;
				if(write_max<=0) {
					log_message_error("Verifying the TCP buffer",PROGRAM,"The buffer is incorrect");
					FD_CLR(client_request[i].socket, &globalwfds);
					ret = shut_and_close(&client_request[i]);
					if(ret==0) {
						return;
					}
					snprintf(concat,1024,"Socket %d (%s), port %d, is now closed",i,inet_ntoa(client_request[i].client_socket.sin_addr),port);
					log_message("Closing TCP client socket",concat);
					set_max_nfds(&nfds, sockfd, client_request);
					continue;
				}
				ret = write(client_request[i].socket, client_request[i].buffer+client_request[i].position,write_max);
				if(ret==-1) {
					log_message_error("Sending data to TCP client",SYSTEM,"write");
					FD_CLR(client_request[i].socket, &globalwfds);
					ret = shut_and_close(&client_request[i]);
					if(ret==0) {
						return;
					}
					snprintf(concat,1024,"Socket %d (%s), port %d, is now closed",i,inet_ntoa(client_request[i].client_socket.sin_addr),port);
					log_message("Closing TCP client socket",concat);
					set_max_nfds(&nfds, sockfd, client_request);
				}
				if(ret<write_max) {
					client_request[i].position = client_request[i].position + ret;
				} else {
					FD_CLR(client_request[i].socket, &globalwfds);
					if(client_request[i].close_request) {
						ret = shut_and_close(&client_request[i]);
						if(ret==0) {
							return;
						}
						snprintf(concat,1024,"Socket %d (%s), port %d, is now closed",i,inet_ntoa(client_request[i].client_socket.sin_addr),port);
						log_message("Closing TCP client socket",concat);
						set_max_nfds(&nfds, sockfd, client_request);
					} else {
						FD_SET(client_request[i].socket, &globalrfds);
					}
				}
			}
		}
	}
	return;
}
