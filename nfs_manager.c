/* nfs_manager */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <arpa/inet.h>


#include "log_funcs.h"
#include "commands.h"

#define WORKER_LIMIT 5
#define MAX_LINES 50
#define MAX_STR_LEN 100
#define MAX_ENTRIES 100
#define MAX_LENGTH 100

//This functio helps to communicate with clients
void connect_to_port(const char *host, const char *port, char* buff){
    int s4 = socket(AF_INET, SOCK_STREAM, 0);   //socket to communicate with clients
    if(s4 < 0){
        perror("Socket error");
        return;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(port));  

    if(inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0){ //converting IP from string to binary format
        perror("Invalid address / Address not supported");
        close(s4);
        return;
    }

    if(connect(s4, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){  //connection with client
        perror("Connection failed");
        close(s4);
        return;
    }

    char *message = buff;
    send(s4, message, strlen(message), 0);      //sends message to client

    close(s4);
}

/* ./nfs_manager -l <manager_logfile> -c <config_file> -n <worker_limit> -p <port_number> -b <bufferSize> */
int main(int argc, char* argv[]){
    char* logfile = NULL;
    char* config_file = NULL;
    int worker_limit = 0;
    int port_num = -1;
    int buff_size = -1;

    char source_array[MAX_ENTRIES][MAX_LENGTH];
    char target_array[MAX_ENTRIES][MAX_LENGTH];
    int count = 0;

    //checking for the arguments
    if(argc != 11)
        exit(EXIT_FAILURE);
    
    if(strcmp(argv[1], "-l") != 0 || strcmp(argv[3], "-c") != 0 || strcmp(argv[5], "-n") != 0 || strcmp(argv[7], "-p") != 0 || strcmp(argv[9], "-b") != 0){
        fprintf(stderr, "Usage: %s -l <manager_logfile> -c <config_file> -n <worker_limit> -p <port_number> -b <bufferSize>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    logfile = argv[2];
    config_file = argv[4];
    worker_limit = atoi(argv[6]);
    port_num = atoi(argv[8]);
    buff_size = atoi(argv[10]); 

    if(worker_limit != WORKER_LIMIT)
        worker_limit = WORKER_LIMIT;
    
    if(!port_num){
        fprintf(stderr, "Port number isn't a number!!\n");
        exit(EXIT_FAILURE);
    }

    if(!buff_size){
        fprintf(stderr, "Buffer size should be a number!!\n");
        exit(EXIT_FAILURE);
    }

    int s2 = socket(AF_INET, SOCK_STREAM, 0);   //creates socket
    if(s2 < 0){
        error_func("Socket Error");
    }

    struct sockaddr_in  servadd;
    memset(&servadd, 0, sizeof(servadd));
    servadd.sin_port = htons(port_num);
    servadd.sin_family = AF_INET;
    servadd.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(s2, (struct sockaddr *)&servadd, sizeof(servadd))){     //binds address to socket
        error_func("Bind Error");
        close(s2);
    }

    if(listen(s2, 5) < 0){      //listens for connections with Qsize=5
        error_func("Listen Error");
        close(s2);
    }

    bool flag = true;
    while(1){
        struct sockaddr_in client;
        socklen_t client_size = sizeof(client);
        int s3 = accept(s2, (struct sockaddr *)&client, &client_size);      //accepts connection to communicate with console
        if(s3 < 0){
            perror("Accept Error");
            continue;               
        }

        char buff[1024];
        while(1){
            memset(buff, 0, sizeof(buff));  //clears the entire board
            int response = recv(s3, buff, sizeof(buff) - 1, 0);         //gets the command
            if(response <= 0){
                break;
            }

            FILE *log = fopen(logfile, "a");       
            if(log){
                fclose(log);
            }
            

            char source[100];
            char target[100];
            buff[response] = '\0';

            char source_path[MAX_LINES][MAX_STR_LEN];
            char source_host_id[MAX_LINES][MAX_STR_LEN];
            char source_port_id[MAX_LINES][MAX_STR_LEN];
            if(!strcmp(buff, "shutdown")){          //checks if the command is shutdown and closes every port that is open
                char buffer[2 * MAX_STR_LEN];
                char first_column[MAX_LINES][MAX_STR_LEN];
                char second_column[MAX_LINES][MAX_STR_LEN];
                char source_path1[MAX_LINES][MAX_STR_LEN];
                char source_host_id1[MAX_LINES][MAX_STR_LEN];
                char source_port_id1[MAX_LINES][MAX_STR_LEN];
                int i = 0;
                
                FILE *file1 = fopen(config_file, "r");
                if(file1 == NULL){
                    perror("Can't read file");
                    return 1;
                }

                while(fgets(buffer, sizeof(buffer), file1) != NULL && i < MAX_LINES){
                    if(sscanf(buffer, "%s %s", first_column[i], second_column[i]) == 2){  //splits the line from the config_file into two words to take source and target
                        i++;
                    } 
                }

                fclose(file1);
                int c = 0; 
                for(int j=0; j<i; j++){
                    if(sscanf(first_column[j], "%[^@]@%[^:]:%s", source_path1[c], source_host_id1[c], source_port_id1[c]) == 3){    //splits the data than we can take from source  
                        c++;
                    }        
                }
                
                for(int k=0; k<count; k++){
                    for(int j=0; j<c; j++){
                        if(strcmp(source_port_id1[j],source_port_id[k]) == 0){
                            connect_to_port(source_host_id1[j], source_port_id1[j], buff);  //calls the function to communicate with client
                        }
                    }
                }

                shutdown_mode(s3);
                flag = false;
            }
            else if(!strncmp(buff, "add ", 4)){     //checks if command is add and prints its message
                if(sscanf(buff + 4, "%s %s", source, target) == 2){
                    strcpy(source_array[count], source);
                    strcpy(target_array[count], target);
                    
                    if(sscanf(source_array[count], "%[^@]@%[^:]:%s", source_path[count], source_host_id[count], source_port_id[count]) == 3){  //splits the data than we can take from source 
                        connect_to_port(source_host_id[count], source_port_id[count], buff);  //calls the function to communicate with client
                    }      
                    add_mode(s3, source_array[count], target_array[count]);
                    printf("\n");
                    count++;
                } 
            }
        }
        if(flag == false){  //if message is shutdown it stops
            break;
        }
        close(s3);  
    }

    close(s2);
    return 0;
}