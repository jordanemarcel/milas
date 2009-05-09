#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "../includes/tcp.h"
#include "../includes/midnet.h"
#include "../includes/log.h"
#include "../includes/midas_utils.h"

int midnet_create_structure(struct _tcp_session* client_request) {
	if(client_request==NULL) {
		log_message_error("MidNet: Creating protocol structure",PROGRAM,"NULL TCP structure");
		return 0;
	}
	return 1;
}

int is_valid_request(char* request) {
	int ret;
	char* token;
	char* mid_token;
	char* end_token;

	if(request==NULL) {
		log_message_error("MidNet: Verifying request",PROGRAM,"NULL request");
		return 0;
	}

	end_token = strstr(request, "\n\n");
	if(end_token==NULL) {
		log_message_error("MidNet: Verifying request",PROGRAM,"Invalid request, no end detected");
		return 0;
	}

	ret = strcmp(request, "list\n\n");
	if(ret==0) {
		return 1;
	}

	token = strstr(request, "\n");
	if(token==end_token) {
		log_message_error("MidNet: Verifying request",PROGRAM,"Wrong command! 2 Aborted.");
		return 0;
	}

	mid_token = strstr(token+1, "\n");
	if(mid_token!=end_token) {
		log_message_error("MidNet: Verifying request",PROGRAM,"Wrong command! 3 Aborted.");
		return 0;
	}

	*token = '\0';

	ret = strcmp(request, "get");
	if(ret==0) {
		*token = '\n';
		return 1;
	} else {
		ret = strcmp(request, "put");
		if(ret==0) {
			*token = '\n';
			return 1;
		} else {
			log_message_error("MidNet: Verifying request",PROGRAM,"Wrong command! 4 Aborted.");
			return 0;
		}
	}
}

int midnet_processing_request(struct _tcp_session* client_request) {
	fd_set globalrfds;
	fd_set readfds;
	struct timeval tv;
	int nfds;
	int sel;
	FD_ZERO(&globalrfds);
	FD_SET(0, &globalrfds);
	nfds = 0;

	if(client_request==NULL) {
		log_message_error("MidNet: Processing request",PROGRAM,"NULL request or TCP structure");
		return 0;
	}

	char* request = client_request->buffer;

	int ret;
	int size;
	char* end_of_flow;
	int position = 0;

	size = strlen(request);
	if(size<=0) {
		log_message_error("MidNet: Processing request",PROGRAM,"Empty request");
		return 0;
	}

	char* buffer;
	buffer = client_request->buffer;

	ret = write(1, request, size);

	client_request->position = 0;

	do {
		if(position>=BUFFER_LENGTH) {
			log_message_error("MidNet: Processing request",PROGRAM,"Receiving from core: TCP buffer overflow");
			return 0;
		}
		readfds = globalrfds;
		tv.tv_sec = INTER_COMMUNICATION_DELAY_SEC;
		tv.tv_usec = INTER_COMMUNICATION_DELAY_USEC;
		sel = select(nfds+1, &readfds, NULL, NULL, &tv);
		if(sel==0) {
			log_message_error("MidNet: Processing request",PROGRAM,"The core server doesn't respond");
			return 0;
		}
		size = read(0, buffer+position, BUFFER_LENGTH - position);
		buffer[position+size] = '\0';
		position = position + size;

		end_of_flow = strstr(buffer, "\n\n");
		if(end_of_flow!=NULL) {
			client_request->position = position;
			return 1;
		}

		if(position==BUFFER_LENGTH) {
			client_request->position = position;
			ret = flush_buffer(client_request);
			if(ret==0) {
				log_message_error("MidNet: Processing request",PROGRAM,"Could not flush the TCP buffer, buffer overflow");
				return 0;
			}
		}
	} while(1);
}

void midnet_set_protocol(struct _tcp_session* client_request) {
	if(client_request==NULL) {
		log_message_error("MidNet: Setting Application Protocol",PROGRAM,"NULL request or TCP structure");
		return;
	}
	client_request->protocol_reader = &midnet_reader;
	client_request->protocol_writer = &midnet_writer;
	client_request->protocol_struct_alloc = &midnet_create_structure;
	client_request->protocol_struct = NULL;
}

int midnet_reader(struct _tcp_session* client_request) {
	char* token;

	if(client_request==NULL) {
		log_message_error("MidNet: Reading data from TCP",PROGRAM,"NULL TCP client structure");
		return -1;
	}
	if((client_request->position)>BUFFER_LENGTH || (client_request->position)<0) {
		log_message_error("MidNet: Reading data from TCP",PROGRAM,"Invalid buffer");
		return -1;
	}

	client_request->buffer[client_request->position] = '\0';

	token = strstr(client_request->buffer, "\n\n");
	if(token==NULL) {
		return 0;
	}
	return 1;
}

int midnet_writer(struct _tcp_session* client_request) {
	int ret;
	char* buffer = client_request->buffer;
	char* token;

	if(client_request==NULL) {
		log_message_error("MidNet: Writing data to TCP",PROGRAM,"NULL TCP client structure");
		return 0;
	}

	token = strstr(client_request->buffer, "\n\n");
	if(token==NULL) {
		log_message_error("MidNet: Writing data to TCP",PROGRAM,"Invalid TCP buffer");
		return 0;
	}

	if(token>=(buffer+BUFFER_LENGTH-1)) {
		log_message_error("MidNet: Writing data to TCP",PROGRAM,"Buffer Overflow");
		return 0;
	}
	*(token+2) = '\0';

	ret = clear_fd(0);
	if(ret==0) {
		log_message_error("MidNet: Writing data to TCP",PROGRAM,"Error while cleaning file descriptor");
		return 0;
	}

	ret = is_valid_request(buffer);
	if(ret==0) {
		log_message_error("MidNet: Writing data to TCP",PROGRAM,"Wrong command");
		return 0;
	}

	ret = midnet_processing_request(client_request);
	if(ret==0) {
		log_message_error("MidNet: Writing data to TCP",PROGRAM,"Error while processing the request");
		return 0;
	}

	client_request->limit = client_request->position;
	client_request->position = 0;
	client_request->close_request = 1;
	return 1;
}
