#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "../includes/tcp.h"
#include "../includes/tcp_client.h"
#include "../includes/log.h"
#include "../includes/list.h"

void list_set_protocol(struct _tcp_session* client_request) {
	if(client_request==NULL) {
		log_message_error("list: Setting Application Protocol",PROGRAM,"NULL request or TCP structure");
		return;
	}
	client_request->protocol_reader = &list_reader;
	client_request->protocol_writer = &list_writer;
	client_request->protocol_struct_alloc = &list_create_structure;
	client_request->protocol_struct = NULL;
}

int list_create_structure(struct _tcp_session* client_request) {
	return 1;
}

int list_reader(struct _tcp_session* client_request) {
	int ret;
	char* word;
	char* token;
	char* end_of_buffer;
	int size;
	client_request->buffer[BUFFER_LENGTH] = '\0';

	end_of_buffer = strstr(client_request->buffer, "\n\n");
	if(end_of_buffer==NULL) {
		end_of_buffer = client_request->buffer + BUFFER_LENGTH;
	}

	word = client_request->buffer;
	token = strstr(word, "\n");
	if(word==token) {
		return 1;
	}
	while(token!=NULL) {
		if(word==token) {
			return 1;
		}
		size = token - word + 1;
		ret = write(1, word, size);
		word = word + size;
		if(word>=end_of_buffer) {
			return 1;
		}
		token = strstr(word, "\n");
	}
	memmove(client_request->buffer, word, end_of_buffer-word);
	client_request->position = end_of_buffer-word;
	return 1;
}

int list_writer(struct _tcp_session* client_request) {
	int ret;
	ret = fill_tcp_buffer(client_request, "list\n\n");
	return ret;
}

int main(int argc, char** argv) {
	launch_tcp_client("127.0.0.1", 5656, &list_set_protocol);
	return 0;
}
