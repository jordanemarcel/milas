#ifndef DEAMON_H
#define DEAMON_H

#define LOG_FILE "/var/log/midas.log"

void stop_daemon(char * pid_filename);
void start_daemon(char * pid_filename);

#endif
