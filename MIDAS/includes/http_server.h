#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CONNECTION 5
#define BUFFER_LENGTH 2048
#define COMMAND_LENGTH 10
#define SELECT_TIMEOUT 10

struct http_request {
	int socket;
	struct sockaddr_in client_socket;
	socklen_t socket_length;
	char buffer[BUFFER_LENGTH];
	int position;
	struct timeval tv;
	char command[COMMAND_LENGTH];
};

void initialize_http_request_all(struct http_request client_request[MAX_CONNECTION]);
void initialize_http_request(struct http_request* client_request);
void shut_and_close_all(struct http_request client_request[MAX_CONNECTION]);
void shut_and_close(struct http_request* client_request);
int verify_data(struct http_request* client_request);
void set_max_nfds(int* nfds, int socketfd, struct http_request client_request[MAX_CONNECTION]);
void launch_http_server(int port);

#endif /* HTTP_SERVER_H_ */


