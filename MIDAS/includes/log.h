#ifndef LOG_H_
#define LOG_H_

enum _error_type {SYSTEM, PROGRAM, WARNING};

int init_log_system(char* file);
int close_log_system();
void write_log_message(char* place);
void log_message(char* place, char* message);
void log_message_error(char* place, enum _error_type error_type, char* message);

#endif /* LOG_H_ */
