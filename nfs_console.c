/* nfs_console */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "log_funcs.h"

/* ./nfs_console -l <console-logfile> -h <host_IP> -p <host_port> */
int main(int argc, char* argv[]){
    char *logfile = NULL;
    char *host_ip = NULL;
    int host_port = -1;

    //checking for the arguments
    if(argc != 7)
        exit(EXIT_FAILURE);

    if(strcmp(argv[1], "-l") != 0 || strcmp(argv[3], "-h") != 0 || strcmp(argv[5], "-p") != 0){
        fprintf(stderr, "Usage: %s -l <console-logfile> -h <host_IP> -p <host_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    logfile = argv[2];
    host_ip = argv[4];
    host_port = atoi(argv[6]);

    if(!host_port){
        fprintf(stderr, "Host port should be a number!!\n");
        exit(EXIT_FAILURE);
    }

    int s1 = socket(AF_INET, SOCK_STREAM, 0);   //creates socket s1
    if(s1 < 0){
        error_func("Socket Error");
    }

    struct sockaddr_in  servadd;
    memset(&servadd, 0, sizeof(servadd));
    servadd.sin_port = htons(host_port);
    servadd.sin_family = AF_INET;

    if(inet_pton(AF_INET, host_ip, &servadd.sin_addr) <= 0){    //converting IP from string to binary format
        error_func("inet_pton error");
        close(s1);
    }

    if(connect(s1, (struct sockaddr*)&servadd, sizeof(servadd)) < 0){  //connection with manager
        error_func("Connect Error");
        close(s1);
    }

    char command[256];
    while(1){
        printf("> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;
    
        if(strlen(command) == 0)
            continue;
        
        if(send(s1, command, strlen(command), 0) < 0){      //sends message to the manager
            perror("Send Error");
            break;
        }

        log_entry_file(command, logfile);

        char buff[1024];
        int responce = recv(s1, buff, sizeof(buff)-1, 0);   //gets responce from manager
        if(responce > 0){
            buff[responce] = '\0';
            printf("%s\n", buff);
            log_entry_file(buff, logfile);
        }

        if (!strcmp(command, "shutdown"))   //if message is shutdown it stops
            break;
    }

    close(s1);
    return 0;
}