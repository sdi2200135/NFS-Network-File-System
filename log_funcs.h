#ifndef LOG_WORKER_UTILS_H
#define LOG_WORKER_UTILS_H

char *get_time();
void error_func(char* mess);
void log_entry(char *entry);
void log_entry_file(char *entry, char* log_file);

#endif