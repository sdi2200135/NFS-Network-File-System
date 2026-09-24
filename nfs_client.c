/* nfs_client */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

/* ./nfs_client -p <port_number> */
int main(int argc, char* argv[]){
    int port_num = -1;

    //checking for the arguments
    if(argc != 3){
        exit(EXIT_FAILURE);
    }

    if(strcmp(argv[1], "-p") != 0){
        fprintf(stderr, "Usage: %s -p <port_number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //get the port num and checking that it is valid 
    port_num = atoi(argv[2]);
    if(port_num <= 0){
        fprintf(stderr, "Port number isn't a number!!\n");
        exit(EXIT_FAILURE);
    }

    int s1 = socket(AF_INET, SOCK_STREAM, 0);   //creates socket for communication with manager
    if(s1 < 0){
        perror("Socket Error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servadd;
    memset(&servadd, 0, sizeof(servadd));
    servadd.sin_port = htons(port_num);
    servadd.sin_family = AF_INET;
    servadd.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(s1, (struct sockaddr *)&servadd, sizeof(servadd))){     //binds address to socket
        perror("Bind Error");
        close(s1);
        exit(EXIT_FAILURE);
    }

    if(listen(s1, 5) < 0){      //listens for connections with Qsize=5
        perror("Listen Error");
        close(s1);
        exit(EXIT_FAILURE);
    }

    while(1){
        struct sockaddr_in client;
        socklen_t client_size = sizeof(client);
        int s3 = accept(s1, (struct sockaddr *)&client, &client_size);      //accepts connection
        if(s3 < 0){
            perror("Accept Error");
            continue;
        }

        char buff[1024];
        int bytes_received = recv(s3, buff, sizeof(buff) - 1, 0);   //gets the message from manager
        if (bytes_received <= 0) {
            perror("Recv Error");
            close(s3);
            continue;
        }
        
        buff[bytes_received] = '\0';
        printf("Received: %s\n", buff);
        if(!strcmp(buff, "shutdown")){      //if message is shutdown it stops
            break;
        }

        close(s3);
    }

    close(s1);
    return 0;
}