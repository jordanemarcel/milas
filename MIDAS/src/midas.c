#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include "../includes/daemon.h"
#include "../includes/tcp_server.h"
#include "../includes/MIDAS_http_server.h"
#include "../includes/midnet.h"
#include "../includes/dictionary.h"
#include "../includes/log.h"
#include "../includes/midas_utils.h"

#define PID_FILE "/var/run/midas.pid"	/* location of the PID file*/

void help() {
	printf("\nMIDAS, Multi Interface Dictionary Access Service\n");
	printf("\n ~ Mon dictionnaire, c'est jamais sans MIDAS ~\n");
	printf("------------------------------------------------\n\n");
	printf("This program manages a dictionary which can be accessed by HTTP or by system programs.\n");
	printf("This program is a daemon. Please use the options to interact with the daemon.\n\n");
	printf("Usage: midas OPTION\n\n");
	printf("Mandatory arguments to long options are mandatory for short options too.\n");
	printf("\t-s, --start\t\t start the daemon\n");
	printf("\t-r, --restart\t\t restart the daemon\n");
	printf("\t-k, --kill\t\t stop the daemon\n");
	printf("\t-h, --help\t\t display this help and exit\n\n");
	printf("Some system programs can manage the dictionary. These programs are:\n");
	printf("\tput\t\t this program puts a word in the dictionary\n");
	printf("\tget\t\t this program tests whether a word is in the dictionary\n");
	printf("\tlist\t\t this program lists the content of the dictionary\n\n");
	printf("This program uses a log file located at: %s\n\n",LOG_FILE);
	printf("INFO: You need to be root in order to start, stop or restart this program.\n\n");
	printf("WARNING: If the program is killed violently, it might appear as running the next time you want to start it.\n");
	printf("If so, please use the restart option 'midas --restart'.\n\n");
}

void execute_request(Dictionary* dictionary, char* request, int fd) {
	int ret;
	int size;
	char* token;
	char return_value[3];
	enum _command command;
	char concat[1024];
	token = strstr(request, "\n");
	if(token==NULL) {
		ret = strcmp(request, "list");
		if(ret==0) {
			ret = write_dictionary(dictionary, fd);
			if(ret==0) {
				log_message_error("Executing request from resource",PROGRAM,"Failed to send the list to the resource");
			}
			log_message("Dictionary","List successfully returned");
		} else {
			log_message_error("Executing request from resource",PROGRAM,"Wrong command! Aborted.");
		}
		return;
	}

	*token = '\0';
	ret = strcmp(request, "get");
	if(ret==0) {
		command = GET;
	} else {
		ret = strcmp(request, "put");
		if(ret==0) {
			command = PUT;
		} else {
			log_message_error("Executing request from resource",PROGRAM,"Wrong command! Aborted.");
			return;
		}
	}

	token = token+1;
	size = strlen(token);
	if(size<1) {
		log_message_error("Executing request from resource",PROGRAM,"Word null. Aborted.");
		return;
	}
	switch(command) {
	case PUT:
		ret = put(dictionary, token);
		switch(ret) {
		case -1:
		log_message_error("Adding a new word to the dictionary",PROGRAM,"Attempt failed.");
		return;
		case 0:
			snprintf(concat,1024,"Word '%s' not added, already exists",token);
			log_message("Dictionary",concat);
			break;
		case 1:
			snprintf(concat,1024,"New word added: '%s'",token);
			log_message("Dictionary",concat);
			break;
		}
		snprintf(return_value,4,"%d\n\n",ret);
		write(fd,return_value,3);
		if(ret==-1) {
			log_message_error("Sending answer of put to resource",SYSTEM,"write");
		}
		return;
	case GET:
		ret = get(dictionary, token);
		switch(ret) {
		case -1:
		log_message_error("Getting a word from the dictionary",PROGRAM,"Attempt failed.");
		return;
		case 0:
			snprintf(concat,1024,"Word '%s' is NOT in the dictionary",token);
			log_message("Dictionary",concat);
			break;
		case 1:
			snprintf(concat,1024,"Word '%s' is in the dictionary",token);
			log_message("Dictionary",concat);
			break;
		}
		snprintf(return_value,4,"%d\n\n",ret);
		write(fd,return_value,3);
		if(ret==-1) {
			log_message_error("Sending answer of get to resource",SYSTEM,"write");
		}
		return;
	default:
		return;
	}
}

void clean_exit_core(int handler) {
	log_message("Stopping the server (core)","done");
	close_log_system();
	exit(EXIT_SUCCESS);
}

void clean_exit_http(int handler) {
	log_message("Stopping the server (HTTP)","done");
	close_log_system();
	exit(EXIT_SUCCESS);
}

void clean_exit_midnet(int handler) {
	log_message("Stopping the server (MidNet)","done");
	close_log_system();
	exit(EXIT_SUCCESS);
}

int set_exit_handler(void (*handler_call)(int handler)) {
	int ret;
	sigset_t set;
	ret = sigemptyset(&set);
	if(ret==1) {
		log_message_error("Emptying handler set",SYSTEM,"sigemptyset");
		return 0;
	}
	ret = sigaddset(&set, SIGTERM);
	if(ret==1) {
		log_message_error("Adding SIGTERM handler",SYSTEM,"sigaddset");
		return 0;
	}
	ret = sigaddset(&set, SIGINT);
	if(ret==1) {
		log_message_error("Adding SIGINT handler",SYSTEM,"sigaddset");
		return 0;
	}
	ret = sigaddset(&set, SIGABRT);
	if(ret==1) {
		log_message_error("Adding SIGABRT handler",SYSTEM,"sigaddset");
		return 0;
	}
	ret = sigaddset(&set, SIGQUIT);
	if(ret==1) {
		log_message_error("Adding SIGQUIT handler",SYSTEM,"sigaddset");
		return 0;
	}
	struct sigaction act;
	act.sa_handler = handler_call;
	act.sa_mask = set;
	act.sa_flags = SA_NODEFER;
	ret = sigaction(SIGTERM, &act, NULL);
	if(ret==1) {
		log_message_error("Setting SIGTERM signal handler",SYSTEM,"sigaction");
		return 0;
	}
	ret = sigaction(SIGINT, &act, NULL);
	if(ret==1) {
		log_message_error("Setting SIGINT signal handler",SYSTEM,"sigaction");
		return 0;
	}
	ret = sigaction(SIGABRT, &act, NULL);
	if(ret==1) {
		log_message_error("Setting SIGABRT signal handler",SYSTEM,"sigaction");
		return 0;
	}
	ret = sigaction(SIGQUIT, &act, NULL);
	if(ret==1) {
		log_message_error("Setting SIGQUIT signal handler",SYSTEM,"sigaction");
		return 0;
	}
	return 1;
}

int main(int argc, char* argv[]) {
	int f;
	int ret;
	char cmd='\0';
	int stderr_fd;
	FILE* stderr_file;
	int val,index = -1;	 /* Use for arg parssing */

	stderr_fd = dup(2);
	if(stderr_fd==-1) {
		fprintf(stderr,"System error while duplicating stderr:\n");
		perror("dup");
		fprintf(stderr,"Aborted.\n");
		exit(EXIT_FAILURE);
	}
	stderr_file = fdopen(stderr_fd, "a");
	if(stderr_file==NULL) {
		fprintf(stderr,"System error while opening stderr:\n");
		perror("fdopen");
		fprintf(stderr,"Aborted.\n");
		exit(EXIT_FAILURE);
	}

	const char* optstring = ":srkh";
	const struct option lopts[] = {
			{"start",no_argument,NULL,'s'},
			{"restart",no_argument,NULL,'r'},
			{"kill",no_argument,NULL,'k'},
			{"help",no_argument,NULL,'h'},
			{NULL, no_argument,NULL,0}
	};
	while(EOF != (val = getopt_long(argc,argv,optstring,lopts,&index))){
		switch(val){
		case 'h':
			help();
			exit(EXIT_SUCCESS);
		case 's':
			cmd='s';
			break;
		case 'r':
			cmd='r';
			break;
		case 'k':
			cmd='k';
			break;
		case ':':
			fprintf(stderr,"Argument missing for %c. Use -h for more information\n",optopt);
			exit(EXIT_FAILURE);
		case '?':
			fprintf(stderr,"Unknown option %c. Use -h for more information\n",optopt);
			exit(EXIT_FAILURE);
		}
	}

	if(cmd=='\0'){
		fprintf(stderr,"midas: missing option.\n");
		fprintf(stderr,"Try 'midas --help' for more information.\n");
		exit(EXIT_FAILURE);
	}
	if(cmd=='s'){
		fprintf(stderr,"Starting the daemon...\n");
		start_daemon(PID_FILE);
	}
	if(cmd=='k'){
		fprintf(stderr,"Stopping the daemon...\n");
		stop_daemon(PID_FILE);
		exit(EXIT_SUCCESS);
	}
	if(cmd=='r'){
		fprintf(stderr,"Restarting the daemon...\n");
		fprintf(stderr,"1. Stopping the daemon...\n");
		stop_daemon(PID_FILE);
		fprintf(stderr,"2. Starting the daemon...\n");
		start_daemon(PID_FILE);
	}

	int pipe_server_to_http[2];
	int pipe_http_to_server[2];
	ret = pipe(pipe_server_to_http);
	if(ret==-1) {
		log_message_error("Creating a pipe",SYSTEM,"pipe");
		log_message_error("Core Server",PROGRAM,"Unexpected error. Shutting down");
		fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
		stop_daemon(PID_FILE);
		exit(EXIT_FAILURE);
	}
	ret = pipe(pipe_http_to_server);
	if(ret==-1) {
		log_message_error("Creating a pipe",SYSTEM,"pipe");
		log_message_error("Core Server",PROGRAM,"Unexpected error. Shutting down");
		fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
		stop_daemon(PID_FILE);
		exit(EXIT_FAILURE);
	}

	f = fork();
	switch(f) {
	case -1:
	perror("fork");
	exit(1);
	case 0:
		log_message("HTTP Server","Initializing");
		ret = set_exit_handler(&clean_exit_http);
		if(ret==0) {
			log_message_error("Configuring signal handler",PROGRAM,"Attempt failed");
			log_message_error("HTTP Server",PROGRAM,"Unexpected error. Shutting down");
			fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
			stop_daemon(PID_FILE);
			exit(EXIT_FAILURE);
		}
		log_message("HTTP Server","Signal handler configured");
		ret = dup2(pipe_server_to_http[0],0);
		if(ret==-1) {
			log_message_error("Piping HTTP server",SYSTEM,"dup2");
			log_message_error("HTTP Server",PROGRAM,"Unexpected error. Shutting down");
			fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
			stop_daemon(PID_FILE);
			exit(EXIT_FAILURE);
		}
		ret = dup2(pipe_http_to_server[1],1);
		if(ret==-1) {
			log_message_error("Piping HTTP server",SYSTEM,"dup2");
			log_message_error("HTTP Server",PROGRAM,"Unexpected error. Shutting down");
			fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
			stop_daemon(PID_FILE);
			exit(EXIT_FAILURE);
		}
		log_message("HTTP Protocol","Creating TCP Server");
		launch_tcp_server(80, &MIDAS_set_protocol);
		log_message_error("HTTP Server",PROGRAM,"Unexpected error. Shutting down");
		fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
		stop_daemon(PID_FILE);
		exit(EXIT_FAILURE);
	}



	int pipe_server_to_midnet[2];
	int pipe_midnet_to_server[2];
	ret = pipe(pipe_server_to_midnet);
	if(ret==-1) {
		log_message_error("Creating a pipe",SYSTEM,"pipe");
		log_message_error("Core Server",PROGRAM,"Unexpected error. Shutting down");
		fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
		stop_daemon(PID_FILE);
		exit(EXIT_FAILURE);
	}
	ret = pipe(pipe_midnet_to_server);
	if(ret==-1) {
		log_message_error("Creating a pipe",SYSTEM,"pipe");
		log_message_error("Core Server",PROGRAM,"Unexpected error. Shutting down");
		fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
		stop_daemon(PID_FILE);
		exit(EXIT_FAILURE);
	}

	f = fork();
	switch(f) {
	case -1:
	perror("fork");
	exit(1);
	case 0:
		log_message("MidNet Server","Initializing");
		ret = set_exit_handler(&clean_exit_midnet);
		if(ret==0) {
			log_message_error("Configuring signal handler",PROGRAM,"Attempt failed");
			log_message_error("MidNet Server",PROGRAM,"Unexpected error. Shutting down");
			fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
			stop_daemon(PID_FILE);
			exit(EXIT_FAILURE);
		}
		log_message("MidNet Server","Signal handler configured");
		ret = dup2(pipe_server_to_midnet[0],0);
		if(ret==-1) {
			log_message_error("Piping MidNet server",SYSTEM,"dup2");
			log_message_error("MidNet Server",PROGRAM,"Unexpected error. Shutting down");
			fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
			stop_daemon(PID_FILE);
			exit(EXIT_FAILURE);
		}
		ret = dup2(pipe_midnet_to_server[1],1);
		if(ret==-1) {
			log_message_error("Piping MidNet server",SYSTEM,"dup2");
			log_message_error("MidNet Server",PROGRAM,"Unexpected error. Shutting down");
			fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
			stop_daemon(PID_FILE);
			exit(EXIT_FAILURE);
		}
		log_message("MidNet Protocol","Creating TCP Server");
		launch_tcp_server(5656, &midnet_set_protocol);
		log_message_error("MidNet Server",PROGRAM,"Unexpected error. Shutting down");
		fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
		stop_daemon(PID_FILE);
		exit(EXIT_FAILURE);
	}





	log_message("Core Server","Initializing");
	ret = set_exit_handler(&clean_exit_core);
	if(ret==0) {
		log_message_error("Configuring signal handler",PROGRAM,"Attempt failed");
		fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
		stop_daemon(PID_FILE);
		exit(EXIT_FAILURE);
	}
	log_message("Core Server","Signal handler configured");

	char buffer[RECEPTION_LENGTH+1];
	char* token;
	int position = 0;
	fd_set globalrfds;
	fd_set readfds;
	struct timeval tv;
	int nfds;
	int sel;
	int current_rfd;
	int current_wfd;
	int listening = 0;
	FD_ZERO(&globalrfds);
	FD_SET(pipe_http_to_server[0], &globalrfds);
	FD_SET(pipe_midnet_to_server[0], &globalrfds);
	if(pipe_http_to_server[0]>pipe_midnet_to_server[0]) {
		nfds = pipe_http_to_server[0];
	} else {
		nfds = pipe_midnet_to_server[0];
	}

	Dictionary dico;
	dico.first = NULL;

	log_message("Core Server","Successfully started");
	while(1) {
		do {
			readfds = globalrfds;
			tv.tv_sec = INTER_COMMUNICATION_DELAY_SEC;
			tv.tv_usec = INTER_COMMUNICATION_DELAY_USEC;
			sel = select(nfds+1, &readfds, NULL, NULL, &tv);
			if(sel==-1) {
				log_message_error("Listening to sockets",SYSTEM,"select");
				break;
			}
			if(listening==1 && sel==0) {
				log_message_error("Core Server",PROGRAM,"Distant resource doesn't respond. Canceling request");
				position = 0;
				listening = 0;
			}
		} while(sel<1);
		if(sel==-1) {
			break;
		}
		if(RECEPTION_LENGTH-position<=0) {
			log_message_error("Core Server",PROGRAM,"Buffer Overflow while reading request. Canceling request");
			position = 0;
			continue;
		}

		if(FD_ISSET(pipe_http_to_server[0], &readfds)) {
			FD_CLR(pipe_midnet_to_server[0], &globalrfds);
			current_rfd = pipe_http_to_server[0];
			current_wfd = pipe_server_to_http[1];
		} else if(FD_ISSET(pipe_midnet_to_server[0], &readfds)) {
			FD_CLR(pipe_http_to_server[0], &globalrfds);
			current_rfd = pipe_midnet_to_server[0];
			current_wfd = pipe_server_to_midnet[1];
		}

		ret = read(current_rfd,buffer+position,RECEPTION_LENGTH-position);
		position = ret + position;
		buffer[position] = '\0';
		token = strstr(buffer, "\n\n");
		if(token==NULL) {
			listening = 1;
			continue;
		}
		*token = '\0';
		execute_request(&dico, buffer, current_wfd);
		position = 0;
		listening = 0;
		FD_ZERO(&globalrfds);
		FD_SET(pipe_http_to_server[0], &globalrfds);
		FD_SET(pipe_midnet_to_server[0], &globalrfds);
	}

	log_message_error("Core Server",PROGRAM,"Unexpected error. Shutting down");
	fprintf(stderr_file,"Unexpected error. Please look at the log file to identify the cause.\n");
	stop_daemon(PID_FILE);
	exit(EXIT_FAILURE);

}
