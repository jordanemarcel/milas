#ifndef MIDAS_HTTP_SERVER_H_
#define MIDAS_HTTP_SERVER_H_

#include "../includes/midas_utils.h"

#define GET_REQUEST_MAX_LENGTH 105

void MIDAS_set_protocol(struct _tcp_session* client_request);
int MIDAS_http_reader(struct _tcp_session* client_request);
int MIDAS_http_writer(struct _tcp_session* client_request);
int MIDAS_create_structure(struct _tcp_session* client_request);
int MIDAS_extract_request(char* request, char* data);
int MIDAS_copy_static(struct _tcp_session* client_request, enum _command command, int end);
int MIDAS_fill_tcp_buffer(struct _tcp_session* client_request, const char* string);
int MIDAS_processing_request(struct _tcp_session* client_request, char* request);
int MIDAS_flush_buffer(struct _tcp_session* client_request);
void MIDAS_flush_fd();
int MIDAS_parse_request(char* request, char** word);

#endif /* MIDAS_HTTP_SERVER_H_ */
