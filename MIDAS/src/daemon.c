#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "../includes/daemon.h"
#include "../includes/log.h"

void stop_daemon(char * pid_filename){
	pid_t pid_fd;			/* The fd descriptor for the pid file */
	int ret;				/* Store the return result of system call */
	char pid_number[6];		/* String that contains the pid number of the deamon */

	pid_fd = open(pid_filename, O_RDWR);
	if(pid_fd==-1) {
		switch(errno){
		case ENOENT:
			fprintf(stderr,"The daemon is not running. Aborted.\n");
			return;
		case EACCES:
			fprintf(stderr,"Failed to stop the daemon. You need to be root.\n");
			break;
		default:
			fprintf(stderr,"Failed to stop the daemon. Aborted.\n");
			fprintf(stderr,"[Opening the PID file] [System error] ");
			perror("open");
		}
		exit(EXIT_FAILURE);
	}

	int tmp=6;
	int pid;
	while((ret = read(pid_fd,pid_number,tmp))!=0){
		if(ret == -1){
			fprintf(stderr,"Failed to read the PID file. Aborted.\n");
			fprintf(stderr,"[Reading PID file: %s] [System error] ",pid_filename);
			perror("read");
			exit(EXIT_FAILURE);
		}
		tmp = tmp - ret;
	}

	ret = unlink(pid_filename);
	if(ret == -1){
		switch(errno){
		case EACCES:
			fprintf(stderr,"Failed to stop the daemon. You need to be root.\n");
			break;
		default:
			fprintf(stderr,"Failed to stop the daemon. Aborted.\n");
			fprintf(stderr,"[Removing PID file: %s] [System error] ",pid_filename);
			perror("unlink");
		}
		exit(EXIT_FAILURE);
	}
	sscanf(pid_number,"%d",&pid);
	ret = kill(-pid,15);
	if(ret == -1){
		switch(errno){
		case EPERM:
			fprintf(stderr,"Failed to stop the daemon. You need to be root.\n");
			break;
		case ESRCH:
			fprintf(stderr,"The daemon is not running. Aborted.\n");
			return;
		default:
			fprintf(stderr,"Failed to stop the daemon. Aborted.\n");
			fprintf(stderr,"[Killing the process] [System error] ");
			perror("kill");
		}
		exit(EXIT_FAILURE);
	}
	usleep(300000);
	fprintf(stderr,"Stopping the daemon: Done!\n");
}

void start_daemon(char * pid_filename){
	pid_t pid_fd;			/* The fd descriptor for the pid file */
	int ret;				/* Store the return result of system call */
	char pid_number[6];		/* String that contains the pid number of the deamon */
	pid_t  pid, sid;		/* The parent and new session process id */

	pid = fork();
	if(pid == -1){
		fprintf(stderr,"Failed to start the daemon. Aborted.\n");
		fprintf(stderr,"[System error] ");
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if(pid>0) {
		usleep(50000);
		exit(EXIT_SUCCESS);
	}

	/* change the file access */
	umask(0);

	/* setsid() detaches and puts the daemon into a new session */
	sid = setsid();
	if (sid == -1) {
		fprintf(stderr,"Failed to start the daemon. Aborted.\n");
		fprintf(stderr,"[System error] ");
		perror("setsid");
		exit(EXIT_FAILURE);
	}
	if ((chdir("/")) == -1) {
		fprintf(stderr,"Failed to start the daemon. Aborted.\n");
		fprintf(stderr,"[System error] ");
		perror("chdir");
		exit(EXIT_FAILURE);
	}

	/* try to create the file in /var/run/midas.pid */
	/* check if this file exist by open it w/o O_CREAT */
	pid_fd = open(pid_filename, O_RDWR);
	/* if open work the file exist so deamon is running */
	if(pid_fd != -1){
		fprintf(stderr,"The daemon is already running. If not, restart it.\n");
		fprintf(stderr,"Try 'midas --help' for more information.\n");
		ret = close(pid_fd);
		if(ret == -1){
			fprintf(stderr,"Failed to close the PID file.\n");
			fprintf(stderr,"[Warning] ");
			perror("close");
		}
		exit(EXIT_FAILURE);
	} else {
		/* if it s fail BUT it's NOT because the file doesn't exist */
		if(errno == EACCES){
			fprintf(stderr,"Failed to start the daemon. You need to be root.\n");
			exit(EXIT_FAILURE);
		} else if(errno != ENOENT){
			fprintf(stderr,"Failed to start the daemon. Aborted.\n");
			fprintf(stderr,"[Opening PID file: %s] [System error] ",pid_filename);
			perror("open");
			exit(EXIT_FAILURE);
		}
		pid_fd = open(pid_filename, O_CREAT | O_WRONLY, S_IRWXU);
		if(pid_fd == -1){
			if(errno == EACCES){
				fprintf(stderr,"Failed to start the daemon. You need to be root.\n");
			}else{
				fprintf(stderr,"Failed to start the daemon. Aborted.\n");
				fprintf(stderr,"[Opening PID file: %s] [System error] ",pid_filename);
				perror("open");
			}
			exit(EXIT_FAILURE);
		}
		ret = sprintf(pid_number,"%d",getpid());
		ret = write(pid_fd,pid_number,ret);
		if(ret == -1){
			fprintf(stderr,"Failed to initialize the PID file. Aborted.\n");
			fprintf(stderr,"[System error] ");
			perror("write");
			exit(EXIT_FAILURE);
		}
		ret = close(pid_fd);
		if(ret == -1){
			fprintf(stderr,"Failed to close the PID file.\n");
			fprintf(stderr,"[Warning] ");
			perror("close");
			exit(EXIT_FAILURE);
		}
	}

	fprintf(stderr,"Starting the daemon: Done!\n");

	ret = init_log_system(LOG_FILE);
	if(ret==0) {
		fprintf(stderr,"Failed to initialize the logger.\n");
		fprintf(stderr,"stderr is now the default output to log messages.\n");
	}
	log_message("Starting the daemon","Done");
}
