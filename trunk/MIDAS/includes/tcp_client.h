#ifndef TCP_CLIENT_H_
#define TCP_CLIENT_H_

int launch_tcp_client(char* address, int port, void (*protocol_initializer)(struct _tcp_session*));

#endif /* TCP_CLIENT_H_ */
