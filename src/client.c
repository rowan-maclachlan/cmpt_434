// Rowan MacLachlan
// rdm695 22265820
// CMPT 434 Derek Eager
// January 25th, 6pm

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "common.h"

#define PORT_SIZE 16
#define HOSTNAME_SIZE 64
#define USAGE "[get|put|quit] filenamesource filenamedest"

int _put(int sockfd, struct command *cmd) {
    FILE *file;
    char cmd_buf[CMD_LIMIT];
    char file_buf[MAX_DATA_SIZE];
    size_t n_read;
    size_t sz;

    file = fopen(cmd->src, "r");
    if (NULL == file) {
        fprintf(stderr, "Failed to open file %s.\n", cmd->src);
        return -1;
    }

    fseek(file, 0L, SEEK_END);
    sz = ftell(file);
    fseek(file, 0L, SEEK_SET); 
    if (sz > MAX_DATA_SIZE) {
        fprintf(stderr, "Filesize exceeds limits.\n");
        return -1;
    }
    
    cmd->fsz = sz;
    serialize_cmd(cmd_buf, cmd); 
    send(sockfd, cmd_buf, strlen(cmd_buf), 0);

    while ((n_read = fread(file_buf, 1, sizeof file_buf, file)) > 0) {
        send(sockfd, file_buf, n_read, 0);
    }

    fclose(file);

    return 0;
}

int _get(int sockfd, struct command *cmd) {
    return 0;
}

int main(int argc, char **argv) {
    int sockfd;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *p;
    char s[INET6_ADDRSTRLEN];
    int status;
    char port[PORT_SIZE];
    char hostname[HOSTNAME_SIZE];
    char cmd_buf[CMD_LIMIT];
    struct command *cmd;
  
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    if (argc != 3) {
        printf("Usage: %s <host name> <port number>", argv[0]);
        exit(1);
    }
    else {
        strlcpy(hostname, argv[1], strlen(argv[1])+1);
        strlcpy(port, argv[2], strlen(argv[2])+1);
        printf("Using hostname %s and port %s\n", hostname, port);
    } 
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }
    
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }
        
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(2);
    }
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connecting to %s\n", s);
    
    freeaddrinfo(servinfo); // all done with this structure
    
    while(1) {
        printf("Enter command of the form '%s':\n", USAGE);

        cmd = deserialize_cmd(get_input(cmd_buf));
        if (NULL == cmd || cmd->type == INV) {
            fprintf(stderr, "Poorly formed command.  Try again.\n");
            continue;
        }            

        if (cmd->type == PUT) {
            _put(sockfd, cmd);
        }
        else if (cmd->type == GET) {
            _get(sockfd, cmd);
        }
        else {
           free(cmd);
           break;
        }

        free(cmd);
    }
    
    close(sockfd);
  
    exit(EXIT_SUCCESS);
}
