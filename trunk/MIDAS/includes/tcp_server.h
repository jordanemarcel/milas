#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include "../includes/tcp.h"

#define MAX_CONNECTION 5
#define BUFFER_LENGTH 1448

int initialize_tcp_request_all(struct _tcp_session client_request[MAX_CONNECTION]);
int shut_and_close_all(struct _tcp_session client_request[MAX_CONNECTION]);
void set_max_nfds(int* nfds, int socketfd, struct _tcp_session client_request[MAX_CONNECTION]);
void set_protocol_initializer(struct _tcp_session client_request[MAX_CONNECTION], void (*protocol_initializer)(struct _tcp_session*));
void launch_tcp_server(int port, void (*protocol_initializer)(struct _tcp_session*));

#endif /* TCP_SERVER_H_ */


