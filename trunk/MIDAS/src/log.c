#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "../includes/log.h"

int init_log_system(char* file) {
	int ret;
	int fd;
	fd = open(file, O_RDWR|O_CREAT|O_APPEND);
	if(fd==-1) {
		log_message_error("Opening the log file",SYSTEM,"open");
		return 0;
	}
	ret = dup2(fd, 2);
	if(ret==-1) {
		log_message_error("Duplicating the log file descriptor",SYSTEM,"dup2");
		return 0;
	}
	ret = close(fd);
	if(ret==-1) {
		log_message_error("Closing the log file descriptor",WARNING,"Attempt failed");
	}
	return 1;
}

int close_log_system() {
	int ret;
	ret = close(2);
	if(ret==-1) {
		log_message_error("Closing the log file descriptor",SYSTEM,"close");
		return 0;
	}
	return 1;
}

void write_log_message(char* place) {
	char string_date[18];
	int error = 0;
	int ret;
	time_t current_time;
	struct tm* cdate;
	int mday,mon,year,hour,min,sec;
	current_time = time(NULL);
	if(current_time==-1) {
		error = 1;
	} else {
		cdate = localtime(&current_time);
		if(cdate==NULL) {
			error = 1;
		} else {
			mday = cdate->tm_mday;
			mon = cdate->tm_mon + 1;
			year = cdate->tm_year - 100;
			hour = cdate->tm_hour;
			min = cdate->tm_min;
			sec = cdate->tm_sec;
			ret = sprintf(string_date,"%02d/%02d/%02d %02d:%02d:%02d",mday,mon,year,hour,min,sec);
			if(ret<0) {
				error = 1;
			}
		}
	}
	if(error==1) {
		fprintf(stderr,"**/**/** **:**:** [Logging a message] [Warning] Failed to get the time.\n");
		strcpy(string_date, "**/**/** **:**:**");
	}
	fprintf(stderr,"%s [%s] ",string_date,place);
}

void log_message(char* place, char* message) {
	write_log_message(place);
	fprintf(stderr,"%s\n",message);
}

void log_message_error(char* place, enum _error_type error_type, char* message) {
	int _errno = errno;
	char* s_error;
	write_log_message(place);
	switch(error_type) {
	case SYSTEM:
		s_error = "System error";
		break;
	case PROGRAM:
		s_error = "Program error";
		break;
	case WARNING:
		s_error = "Warning";
		break;
	default:
		s_error = "Error";
		break;
	}
	fprintf(stderr,"[%s] ",s_error);
	switch(error_type) {
	case SYSTEM:
		errno = _errno;
		perror(message);
		break;
	default:
		fprintf(stderr,"%s\n",message);
		break;
	}
}
