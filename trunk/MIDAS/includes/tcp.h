#ifndef TCP_H_
#define TCP_H_

#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_LENGTH 1448
#define SELECT_TIMEOUT 3

struct _tcp_session {
	int socket;
	struct sockaddr_in client_socket;
	socklen_t socket_length;
	char buffer[BUFFER_LENGTH+1];
	int position;
	int limit;
	struct timeval tv;
	int close_request;
	int (*protocol_reader)(struct _tcp_session*);
	int (*protocol_writer)(struct _tcp_session*);
	int (*protocol_struct_alloc)(struct _tcp_session*);
	void* protocol_struct;
};

int initialize_tcp_request(struct _tcp_session* client_request);
int shut_and_close(struct _tcp_session* client_request);
int fill_tcp_buffer(struct _tcp_session* client_request, const char* string);
int flush_buffer(struct _tcp_session* client_request);

#endif /* TCP_H_ */
