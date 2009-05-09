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
#include <netdb.h>

#include "../includes/tcp.h"
#include "../includes/log.h"

int launch_tcp_client(char* address, int port, void (*protocol_initializer)(struct _tcp_session*)) {
	int ret;

	int sockfd;
	struct _tcp_session client_request;

	protocol_initializer(&client_request);

	ret = initialize_tcp_request(&client_request);
	if(ret==0) {
		return 0;
	}

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sockfd==-1) {
		log_message_error("Creating TCP socket",SYSTEM,"socket");
		return 0;
	}

	struct sockaddr_in client_addr;
	struct hostent* host;
	host = gethostbyname(address);
	client_addr.sin_family = PF_INET;
	client_addr.sin_port = htons(port);
	memcpy(&client_addr.sin_addr, host->h_addr_list[0], host->h_length);

	ret = connect(sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr));
	if(ret==-1) {
		log_message_error("Connecting to TCP socket",SYSTEM,"connect");
		return 0;
	}

	ret = client_request.protocol_writer(&client_request);
	if(ret==0) {
		log_message_error("Writing to TCP socket",PROGRAM,"Attempt failed");
		return 0;
	}
	ret = write(sockfd, client_request.buffer, client_request.position);
	if(ret==0) {
		log_message_error("Writing to TCP socket",SYSTEM,"write");
		return 0;
	}

	char* token;
	fd_set globalrfds;
	fd_set readfds;
	struct timeval tv;
	int nfds;
	int sel;
	FD_ZERO(&globalrfds);
	FD_SET(sockfd, &globalrfds);
	nfds = sockfd;

	client_request.position = 0;

	do {
		readfds = globalrfds;
		tv.tv_sec = SELECT_TIMEOUT;
		tv.tv_usec = 0;
		sel = select(nfds+1, &readfds, NULL, NULL, &tv);
		if(sel==-1) {
			log_message_error("Listening to sockets",SYSTEM,"select");
			return 0;
		}
		if(sel==0) {
			log_message_error("list",PROGRAM,"Distant resource doesn't respond. Canceling request");
			return 0;
		}
		if(BUFFER_LENGTH-(client_request.position)<=0) {
			log_message_error("list",PROGRAM,"Buffer Overflow while reading request. Canceling request");
			return 0;
		}
		ret = read(sockfd,client_request.buffer+client_request.position,BUFFER_LENGTH-client_request.position);
		if(ret==0) {
			ret = client_request.protocol_reader(&client_request);
			if(ret==0) {
				log_message_error("list",PROGRAM,"Error while reading data from core");
				return 0;
			}
			continue;
		}
		client_request.position = ret + client_request.position;
		client_request.buffer[client_request.position] = '\0';
		token = strstr(client_request.buffer, "\n\n");
		if(token==NULL) {
			continue;
		}
		*(token+2) = '\0';
		break;
	} while(sel>0);

	ret = client_request.protocol_reader(&client_request);
	if(ret==0) {
		log_message_error("list",PROGRAM,"Error while reading data from core");
		return 0;
	}
	return 1;
}
