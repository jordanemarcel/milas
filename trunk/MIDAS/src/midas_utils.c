#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "../includes/midas_utils.h"
#include "../includes/log.h"

int clear_fd(int fd) {
	if(fd<0) {
		log_message_error("Clearing file descriptor",PROGRAM,"Invalid file descriptor");
		return 0;
	}

	int ret;
	char buffer[RECEPTION_LENGTH+1];

	fd_set globalrfds;
	fd_set readfds;
	struct timeval tv;
	int nfds;
	int sel;
	FD_ZERO(&globalrfds);
	FD_SET(fd, &globalrfds);
	nfds = fd;

	do {
		readfds = globalrfds;
		tv.tv_sec = FLUSH_STDIN_DELAY_SEC;
		tv.tv_usec = FLUSH_STDIN_DELAY_USEC;
		sel = select(nfds+1, &readfds, NULL, NULL, &tv);
		if(sel==-1) {
			log_message_error("Clearing file descriptor",SYSTEM,"select");
			return 0;
		}
		if(sel>0) {
			ret = read(fd,buffer,RECEPTION_LENGTH);
			if(ret==-1) {
				log_message_error("Clearing file descriptor",SYSTEM,"read");
				return 0;
			}
		}
	} while(sel>0);
	return 1;
}
