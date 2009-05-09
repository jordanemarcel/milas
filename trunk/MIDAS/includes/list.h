#ifndef LIST_H_
#define LIST_H_

void list_set_protocol(struct _tcp_session* client_request);
int list_create_structure(struct _tcp_session* client_request);
int list_reader(struct _tcp_session* client_request);
int list_writer(struct _tcp_session* client_request);

#endif /* LIST_H_ */
