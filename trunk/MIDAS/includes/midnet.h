#ifndef MIDNET_H_
#define MIDNET_H_

int midnet_create_structure(struct _tcp_session* client_request);
int is_valid_request(char* request);
int midnet_processing_request(struct _tcp_session* client_request);
void midnet_set_protocol(struct _tcp_session* client_request);
int midnet_reader(struct _tcp_session* client_request);
int midnet_writer(struct _tcp_session* client_request);

#endif /* MIDNET_H_ */
