/* commands */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>

#include "commands.h" 

//These functions print data 

void shutdown_mode(int s3){
    char response_msg[1024];
    response_msg[0] = '\0';  // clear buffer

    char* timestamp = get_time();   //gets time

    //creates the messages to send them
    snprintf(response_msg + strlen(response_msg), sizeof(response_msg) - strlen(response_msg), "[%s] Shutting down manager...\n", timestamp);
    snprintf(response_msg + strlen(response_msg), sizeof(response_msg) - strlen(response_msg), "[%s] Waiting for all active workers to finish.\n", timestamp);
    snprintf(response_msg + strlen(response_msg), sizeof(response_msg) - strlen(response_msg), "[%s] Processing remaining queued tasks.\n", timestamp);
    snprintf(response_msg + strlen(response_msg), sizeof(response_msg) - strlen(response_msg), "[%s] Manager shutdown complete.\n", timestamp);

    printf("%s", response_msg);     //prints to screen

    if(send(s3, response_msg, strlen(response_msg), 0) < 0){    //sends to console
        perror("Send Error");
    }
}

void add_mode(int s3, char* source, char* target){
    char response_msg[1024];
    response_msg[0] = '\0';  // clear buffer

    char* timestamp = get_time();   //gets time

    snprintf(response_msg, sizeof(response_msg), "[%s] Added file %s -> %s", timestamp, source, target);     //sends response message
    
    printf("%s", response_msg); //prints to screen
    
    log_entry_file(response_msg, "manager_logfile");    //prints to manager_logfile
    
    if(send(s3, response_msg, strlen(response_msg), 0) < 0){    //sends to console
        perror("Send Error");
    }
}