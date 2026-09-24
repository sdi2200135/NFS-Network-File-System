/* log_funcs */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log_funcs.h"

//This function gets local time and date 
char *get_time(){
    static char timestamp[20];      //keeps it in form "YYYY-MM-DD HH:MM:SS"
    time_t now = time(NULL);        //timestamp now
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));   //Formatting the date and time
    return timestamp;
}

//This function prints error messages
void error_func(char* mess){
    perror(mess);
    exit(EXIT_FAILURE);
}

//This function prints a message to the console along with a timestamp
void log_entry(char *entry){
    char* timestamp = get_time();
    printf("[%s] %s\n", timestamp, entry); 
}

//This function prints a message to the log file along with a timestamp
void log_entry_file(char *entry, char* log_file){
    FILE *log = fopen(log_file, "a");   //open the log file
    if(!log) 
        return;

    char* timestamp = get_time();
    
    if(!strncmp(log_file, "console_logfile", strlen("console_logfile"))){
        //prints the message in different way according to its form
        if (!strcmp(entry, "shutdown") || !strncmp(entry, "add ", 4) || !strncmp(entry, "cancel ", 7))
            fprintf(log, "[%s] Command %s\n", timestamp, entry);
        else
            fprintf(log, "%s\n", entry);
    }
    else if(!strncmp(log_file, "manager_logfile", strlen("manager_logfile"))){
        fprintf(log, "%s\n", entry);
    }

    fclose(log);
}
