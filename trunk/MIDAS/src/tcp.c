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
#include "../includes/log.h"

int initialize_tcp_request(struct _tcp_session* client_request) {
	client_request->socket = -1;
	client_request->socket_length = sizeof(client_request->client_socket);
	client_request->tv.tv_sec = 0;
	client_request->tv.tv_usec = 0;
	client_request->position = 0;
	client_request->limit = BUFFER_LENGTH;
	client_request->close_request = 0;
	return client_request->protocol_struct_alloc(client_request);
}

int shut_and_close(struct _tcp_session* client_request) {
	int ret;
	ret = shutdown(client_request->socket, SHUT_RDWR);
	if(ret==-1) {
		log_message_error("Shutting down TCP socket",SYSTEM,"shutdown");
		log_message_error("Shutting down TCP socket",WARNING,"Attempt failed, trying to close");
	}
	ret = close(client_request->socket);
	if(ret==-1) {
		log_message_error("Closing TCP socket",SYSTEM,"close");
		log_message_error("Closing TCP socket",WARNING,"Attempt failed");
	}
	return initialize_tcp_request(client_request);
}

int fill_tcp_buffer(struct _tcp_session* client_request, const char* string) {
	int size;
	int size_available;
	int size_to_copy;
	int size_copied;
	int ret;

	if(client_request==NULL || string==NULL) {
		log_message_error("Filling TCP buffer",PROGRAM,"NULL TCP structure or data");
		return 0;
	}

	size = strlen(string);
	if(size<1) {
		log_message_error("Filling TCP buffer",PROGRAM,"Data is empty");
		return 0;
	}

	size_copied = 0;
	while(size_copied<size) {
		size_available = BUFFER_LENGTH - client_request->position;
		if(size_available >= (size-size_copied)) {
			size_to_copy = size - size_copied;
		} else if(size_available == 0) {
			ret = flush_buffer(client_request);
			if(ret==0) {
				log_message_error("Filling TCP buffer",PROGRAM,"Could not flush the buffer");
				return 0;
			}
			continue;
		} else {
			size_to_copy = size_available;
		}
		strncpy(client_request->buffer+client_request->position, string+size_copied, size_to_copy);
		client_request->position = client_request->position + size_to_copy;
		size_copied = size_copied + size_to_copy;
	}
	return 1;
}

int flush_buffer(struct _tcp_session* client_request) {
	if(client_request==NULL) {
		log_message_error("TCP: Flushing buffer",PROGRAM,"NULL request or TCP structure");
		return 0;
	}
	int ret;
	ret = write(client_request->socket, client_request->buffer,client_request->position);
	client_request->position = 0;
	client_request->limit = BUFFER_LENGTH;
	return ret;
}
