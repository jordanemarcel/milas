#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "../includes/tcp_server.h"
#include "../includes/MIDAS_http_server.h"
#include "../includes/log.h"
#include "../includes/midas_utils.h"

static const char* LIST_REQUEST_START = "\
<html><head>\n\
<title>List of words</title>\n\
<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\
</head><body bgcolor='aqua'>\n\
<h1>List of words from the server:</h1>\n\
<ul>\n";

static const char* LIST_REQUEST_END = "\
</ul><br /><i>End of list</i>\n\
</body></html>\n\0";

static const char* GET_REQUEST_START = "\
<html><head>\n\
<title>Get a word</title>\n\
<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\
</head><body bgcolor='aqua'>\n\
<h1>GET Request: \n";

static const char* GET_REQUEST_OK = "\
<h2><blink>Success</blink></h2>\n\
The word is present in the dictionary.<br />\n\0";

static const char* GET_REQUEST_NOK = "\
<h2><blink>Failure</blink></h2>\n\
The word is NOT present in the dictionary.<br />\n\0";

static const char* GET_REQUEST_END = "\
<br /><a href='list'>List the content of the dictionary</a>\n\
</body></html>\n\0";

static const char* PUT_REQUEST_START = "\
<html><head>\n\
<title>Put a word</title>\n\
<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\
</head><body bgcolor='aqua'>\n\
<h1>PUT Request: \n";

static const char* PUT_REQUEST_OK = "\
<h2><blink>Success</blink></h2>\n\
The word has been added in the dictionary.<br />\n\0";

static const char* PUT_REQUEST_NOK = "\
<h2><blink>Failure</blink></h2>\n\
The word is already in the dictionary.<br />\n\0";

static const char* PUT_REQUEST_END = "\
<br /><br /><a href='list'>List the content of the dictionary</a>\n\
</body></html>\n\0";

static const char* BAD_REQUEST = "\
<html><head>\n\
<title>400 Bad Request</title>\n\
<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\
</head><body bgcolor='red'>\n\
<h1>400 Bad Request</h1>\n\
Your browser sent a request that this server could not understand.<br /><br />\n\
<a href='list'>List the content of the dictionary</a>\n\
</body></html>\n\0";

static const char* INTERNAL_ERROR = "\
<html><head>\n\
<title>500 Server Error</title>\n\
<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\
</head><body bgcolor='red'>\n\
<h1>500 Server Error</h1>\n\
The server didn't respond in time. Too bad.<br />\n\
</body></html>\n\0";

static const char* WELCOME_PAGE = "\
<html><head>\n\
<title>MIDAS v1.0</title>\n\
</head><body bgcolor='aqua'>\n\
<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\
<h1>MIDAS v1.0</h1>\n\
Welcome to MIDAS!<br />\n\
<ul><u>List of commands:</u>\n\
<li><i><b><a href='list'>list</a>: </b>list the content of the dictionary.</i></li>\n\
<li><i><b>get_word: </b>verify whether 'word' is present in the dictionary or not.</i></li>\n\
<li><i><b>put_word: </b>add 'word' to the dictionary. No duplicated word is permitted.</i></li>\n\
</ul>\n\
<small><i>MIDAS - By Clément LEBRETON & Jordane MARCEL</i></small>\n\
</body></html>\n\0";

int MIDAS_extract_request(char* request, char* data) {
	char* ptr;
	int i = 0;

	if(data==NULL) {
		log_message_error("HTTP: Extracting request",PROGRAM,"Can't extract from NULL data");
		return 0;
	}

	ptr = strstr(data, "/");
	if(ptr==NULL) {
		log_message_error("HTTP: Extracting request",PROGRAM,"Unacceptable data (missing '/')");
		return 0;
	}
	ptr++;

	while(((*ptr)!='\0' && i<GET_REQUEST_MAX_LENGTH)) {
		if((*ptr)!=' ' && (*ptr)!='\r') {
			request[i] = (*ptr);
			i++;
		} else {
			break;
		}
		ptr++;
	}
	if(i>GET_REQUEST_MAX_LENGTH) {
		log_message_error("HTTP: Extracting request",PROGRAM,"Request buffer overflow");
		return -1;
	}
	request[i] = '\0';
	return i;
}

int MIDAS_copy_static(struct _tcp_session* client_request, enum _command command, int end) {
	const char* data_block;
	int ret;

	if(client_request==NULL) {
		log_message_error("HTTP: Copying static block",PROGRAM,"NULL TCP structure client_request");
		return 0;
	}

	switch(command) {
	case LIST:
		if(!end) {
			data_block = LIST_REQUEST_START;
		} else {
			data_block = LIST_REQUEST_END;
		}
		break;
	case GET:
		if(!end) {
			data_block = GET_REQUEST_START;
		} else {
			data_block = GET_REQUEST_END;
		}
		break;
	case PUT:
		if(!end) {
			data_block = PUT_REQUEST_START;
		} else {
			data_block = PUT_REQUEST_END;
		}
		break;
	default:
		log_message_error("HTTP: Copying static block",PROGRAM,"Unknown command to copy");
		return 0;
	}

	ret = MIDAS_fill_tcp_buffer(client_request, data_block);
	return ret;
}

int MIDAS_fill_tcp_buffer(struct _tcp_session* client_request, const char* string) {
	int size;
	int size_available;
	int size_to_copy;
	int size_copied;
	int ret;

	if(client_request==NULL || string==NULL) {
		log_message_error("HTTP: Filling TCP buffer",PROGRAM,"NULL TCP structure or data");
		return 0;
	}

	size = strlen(string);
	if(size<1) {
		log_message_error("HTTP: Filling TCP buffer",PROGRAM,"Data is empty");
		return 0;
	}

	size_copied = 0;
	while(size_copied<size) {
		size_available = BUFFER_LENGTH - client_request->position;
		if(size_available >= (size-size_copied)) {
			size_to_copy = size - size_copied;
		} else if(size_available == 0) {
			ret = MIDAS_flush_buffer(client_request);
			if(ret==0) {
				log_message_error("HTTP: Filling TCP buffer",PROGRAM,"Could not flush the buffer");
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

int MIDAS_create_structure(struct _tcp_session* client_request) {
	if(client_request==NULL) {
		log_message_error("HTTP: Creating protocol structure",PROGRAM,"NULL TCP structure");
		return 0;
	}
	return 1;
}

int MIDAS_parse_request(char* request, char** word) {
	int size;
	char* ret;

	if(request==NULL) {
		log_message_error("HTTP: Parsing request",PROGRAM,"NULL request");
		return -1;
	}

	size = strlen(request);
	if(size<1) {
		log_message_error("HTTP: Parsing request",PROGRAM,"Request is empty");
		return -1;
	}

	ret = strstr(request, "list");
	if(ret==request && size==strlen("list")) {
		*word = NULL;
		return LIST;
	}
	ret = strstr(request, "put_");
	if(ret==request && size>strlen("put_")) {
		*word = request + strlen("put_");
		return PUT;
	}
	ret = strstr(request, "get_");
	if(ret==request && size>strlen("get_")) {
		*word = request + strlen("get_");
		return GET;
	}
	log_message_error("HTTP: Parsing request",PROGRAM,"Unknown request or argument needed");
	return -1;
}

int MIDAS_processing_request(struct _tcp_session* client_request, char* request) {
	fd_set globalrfds;
	fd_set readfds;
	struct timeval tv;
	int nfds;
	int sel;
	FD_ZERO(&globalrfds);
	FD_SET(0, &globalrfds);
	nfds = 0;

	if(client_request==NULL || request==NULL) {
		log_message_error("HTTP: Processing request",PROGRAM,"NULL request or TCP structure");
		return -1;
	}

	int ret;
	int ret2;
	int size;
	char reception_buffer[RECEPTION_LENGTH+1];
	char* token;
	char save;
	char* current_flow;
	char* end_of_flow;
	int position = 0;
	enum _command command;

	size = strlen(request);
	if(size<=0) {
		log_message_error("HTTP: Processing request",PROGRAM,"Empty request");
		return 0;
	}

	char* word;
	command = MIDAS_parse_request(request, &word);

	switch(command) {
	case LIST:
		ret = write(1, "list\n", 5);
		if(ret==-1) {
			log_message_error("HTTP: Processing request - LIST",SYSTEM,"write");
			return -1;
		}
		break;
	case GET:
		ret = write(1, "get\n", 4);
		if(ret==-1) {
			log_message_error("HTTP: Processing request - GET",SYSTEM,"write");
			return -1;
		}
		ret = write(1, word, strlen(word));
		if(ret==-1) {
			log_message_error("HTTP: Processing request - GET",SYSTEM,"write");
			return -1;
		}
		ret = write(1, "\n", 1);
		if(ret==-1) {
			log_message_error("HTTP: Processing request - GET",SYSTEM,"write");
			return -1;
		}
		break;
	case PUT:
		ret = write(1, "put\n", 4);
		if(ret==-1) {
			log_message_error("HTTP: Processing request - PUT",SYSTEM,"write");
			return -1;
		}
		ret = write(1, word, strlen(word));
		if(ret==-1) {
			log_message_error("HTTP: Processing request - PUT",SYSTEM,"write");
			return -1;
		}
		ret = write(1, "\n", 1);
		if(ret==-1) {
			log_message_error("HTTP: Processing request - PUT",SYSTEM,"write");
			return -1;
		}
		break;
	default:
		log_message_error("HTTP: Processing request",PROGRAM,"Invalid command");
		return 0;
	}
	ret = write(1, "\n", 1);
	if(ret==-1) {
		log_message_error("HTTP: Processing request - Closing command",SYSTEM,"write");
		return -1;
	}

	client_request->position = 0;
	ret = MIDAS_copy_static(client_request, command, 0);
	if(ret==0) {
		log_message_error("HTTP: Processing request",PROGRAM,"Preparing data: Could not fill in the TCP buffer");
		return -1;
	}

	if(word!=NULL) {
		ret = MIDAS_fill_tcp_buffer(client_request, word);
		if(ret==0) {
			log_message_error("HTTP: Processing request",PROGRAM,"Preparing data: Could not fill in the TCP buffer");
			return -1;
		}
		ret = MIDAS_fill_tcp_buffer(client_request, "</h1>\n");
		if(ret==0) {
			log_message_error("HTTP: Processing request",PROGRAM,"Preparing data: Could not fill in the TCP buffer");
			return -1;
		}
	}

	do {
		if(position>=RECEPTION_LENGTH) {
			log_message_error("HTTP: Processing request",PROGRAM,"Receiving from core: TCP buffer overflow");
			return -1;
		}
		readfds = globalrfds;
		tv.tv_sec = INTER_COMMUNICATION_DELAY_SEC;
		tv.tv_usec = INTER_COMMUNICATION_DELAY_USEC;
		sel = select(nfds+1, &readfds, NULL, NULL, &tv);
		if(sel==0) {
			log_message_error("HTTP: Processing request",PROGRAM,"The core server doesn't respond");
			return -1;
		}
		size = read(0, reception_buffer+position, RECEPTION_LENGTH - position);
		reception_buffer[position+size] = '\0';
		end_of_flow = strstr(reception_buffer, "\n\n");
		if(end_of_flow==NULL) {
			end_of_flow = reception_buffer + RECEPTION_LENGTH;
		}

		token = strchr(reception_buffer, '\n');
		if(token==NULL) {
			position += size;
			continue;
		} else {
			if(token==reception_buffer) {
				break;
			}
		}


		current_flow = reception_buffer;
		do {
			save = *token;
			*token = '\0';
			switch(command) {
			case LIST:
				if(current_flow!=end_of_flow) {
					ret = MIDAS_fill_tcp_buffer(client_request, "<li>");
					if(ret==0) {
						log_message_error("HTTP: Processing request",PROGRAM,"Receiving from core: Could not fill in the TCP buffer");
						return -1;
					}
					ret = MIDAS_fill_tcp_buffer(client_request, current_flow);
					if(ret==0) {
						log_message_error("HTTP: Processing request",PROGRAM,"Receiving from core: Could not fill in the TCP buffer");
						return -1;
					}
					ret = MIDAS_fill_tcp_buffer(client_request, "</li>\n");
					if(ret==0) {
						log_message_error("HTTP: Processing request",PROGRAM,"Receiving from core: Could not fill in the TCP buffer");
						return -1;
					}
				}
				break;
			case PUT:
				sscanf(current_flow,"%d",&ret);
				switch(ret) {
				case 0:
					ret2 = MIDAS_fill_tcp_buffer(client_request, PUT_REQUEST_NOK);
					break;
				case 1:
					ret2 = MIDAS_fill_tcp_buffer(client_request, PUT_REQUEST_OK);
					break;
				default:
					log_message_error("HTTP: Processing request",PROGRAM,"Invalid value received from core server");
					return -1;
				}
				if(ret2==0) {
					log_message_error("HTTP: Processing request",PROGRAM,"Receiving from core: Could not fill in the TCP buffer");
					return -1;
				}
				break;
			case GET:
				sscanf(current_flow,"%d",&ret);
				switch(ret) {
				case 0:
					ret2 = MIDAS_fill_tcp_buffer(client_request, GET_REQUEST_NOK);
					break;
				case 1:
					ret2 = MIDAS_fill_tcp_buffer(client_request, GET_REQUEST_OK);
					break;
				default:
					log_message_error("HTTP: Processing request",PROGRAM,"Invalid value received from core server");
					return -1;
				}
				if(ret2==0) {
					log_message_error("HTTP: Processing request",PROGRAM,"Receiving from core: Could not fill in the TCP buffer");
					return -1;
				}
				break;
			}

			current_flow = token+1;
			token = strchr(current_flow, '\n');
		} while(token!=NULL && current_flow<end_of_flow);

		if(end_of_flow<(reception_buffer + RECEPTION_LENGTH)) {
			break;
		}
		if(current_flow<(reception_buffer+position+size)) {
			ret = reception_buffer+position+size-current_flow;
			memmove(reception_buffer, current_flow, ret);
			reception_buffer[ret] = '\0';
			position = ret;
		} else {
			position = 0;
		}
	} while(size>0);
	ret = MIDAS_copy_static(client_request, command, 1);
	if(ret==0) {
		log_message_error("HTTP: Processing request",PROGRAM,"Closing HTML file: Could not fill in the TCP buffer");
		return -1;
	}
	return 1;
}

int MIDAS_flush_buffer(struct _tcp_session* client_request) {
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

void MIDAS_set_protocol(struct _tcp_session* client_request) {
	if(client_request==NULL) {
		log_message_error("HTTP: Setting Application Protocol",PROGRAM,"NULL request or TCP structure");
		return;
	}
	client_request->protocol_reader = &MIDAS_http_reader;
	client_request->protocol_writer = &MIDAS_http_writer;
	client_request->protocol_struct_alloc = &MIDAS_create_structure;
	client_request->protocol_struct = NULL;
}

int MIDAS_http_reader(struct _tcp_session* client_request) {
	int position = client_request->position;
	char* buffer = client_request->buffer;
	char* token;

	if(position>BUFFER_LENGTH || position<0) {
		log_message_error("HTTP: Reading data from TCP",PROGRAM,"Invalid buffer");
		return -1;
	}

	client_request->buffer[position] = '\0';

	token = strstr(buffer, "\r\n\r\n");
	if(token==NULL) {
		return 0;
	}
	return 1;
}

int MIDAS_http_writer(struct _tcp_session* client_request) {
	int size;
	int ret;
	char request[GET_REQUEST_MAX_LENGTH+1];
	char* buffer = client_request->buffer;
	char* token;
	char save;

	ret = clear_fd(0);
	if(ret==0) {
		log_message_error("HTTP: Writing data to TCP",PROGRAM,"Error while cleaning file descriptor");
		return 0;
	}

	ret = strncmp(buffer, "GET ", 4);
	if(ret!=0) {
		strcpy(buffer, BAD_REQUEST);
		client_request->position = 0;
		client_request->limit = strlen(BAD_REQUEST);
		client_request->close_request = 1;
		return 1;
	}

	token = strstr(buffer, "\r\n");

	save = *token;
	*token = '\0';

	size = MIDAS_extract_request(request, buffer);
	*token = save;

	if(size<1) {
		strcpy(buffer, WELCOME_PAGE);
		log_message("HTTP: Sending response","Welcome page");
		client_request->position = 0;
		client_request->limit = strlen(WELCOME_PAGE);
		client_request->close_request = 1;
		return 1;
	}

	ret = MIDAS_processing_request(client_request, request);
	if(ret==0) {
		strcpy(buffer, BAD_REQUEST);
		log_message("HTTP: Sending response","Bad Request page");
		client_request->position = 0;
		client_request->limit = strlen(BAD_REQUEST);
		client_request->close_request = 1;
		return 1;
	}
	if(ret==-1) {
		strcpy(buffer, INTERNAL_ERROR);
		log_message("HTTP: Sending response","Internal Error page");
		client_request->position = 0;
		client_request->limit = strlen(INTERNAL_ERROR);
		client_request->close_request = 1;
		return 1;
	}
	client_request->limit = client_request->position;
	client_request->position = 0;
	client_request->close_request = 1;
	return 1;
}
