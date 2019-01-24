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
    size_t n_remaining;

    file = fopen(cmd->src, "r");
    if (NULL == file) {
        perror("fopen");
        return -1;
    }

    fseek(file, 0L, SEEK_END);
    cmd->fsz = ftell(file);
    fseek(file, 0L, SEEK_SET);
    if (cmd->fsz > MAX_DATA_SIZE) {
        fprintf(stderr, "Filesize exceeds limits.\n");
        return -1;
    }

    // Send put request
    send_cmd(sockfd, cmd);
    // Recieve handshake
    if (-1 == recv_cmd(sockfd, &cmd)) {
        fprintf(stderr, "Error: client: failed to receive handshake command.\n");
        return -1;
    }

    if (FILE_OK != cmd->err) {
        fprintf(stderr, "Error: client: request: %d.\n", cmd->err);
        return -1;
    }

    // send file contents
    n_remaining = send_file(file, sockfd, cmd->fsz);

    fclose(file);

    return n_remaining;
}

int _get(int sockfd, struct command *cmd) {
    FILE *file;
    size_t n_remaining = 0;

    file = fopen(cmd->dest, "w");
    if (NULL == file) {
        perror("fopen");
        return -1;
    }

    // send get request
    cmd->fsz = 0;
    send_cmd(sockfd, cmd);

    // Recieve handshake
    if (-1 == recv_cmd(sockfd, &cmd)) {
        fprintf(stderr, "Error: client: failed to receive handshake command.\n");
        fclose(file);
        return -1;
    }

    if (FILE_OK != cmd->err) {
        fprintf(stderr, "Error: client: request: %d.\n", cmd->err);
        fclose(file);
        return -1;
    }

    // Receive file contents
    if (0 != (n_remaining = recv_write_file(sockfd, file, cmd->fsz))) {
        fprintf(stderr, "Error: client: failed to receive/write file.\n");
    }

    fclose(file);

    return n_remaining;
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
        strncpy(hostname, argv[1], strlen(argv[1]));
        hostname[strlen(argv[1])] = '\0';
        strncpy(port, argv[2], strlen(argv[2]));
        port[strlen(argv[2])] = '\0';
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

    set_timeout(sockfd);

    while(1) {
        printf("Enter command of the form '%s':\n", USAGE);

        cmd = parse_cmd(get_input(cmd_buf));

        print_cmd(cmd);

        if (NULL == cmd || cmd->type == INV) {
            fprintf(stderr, "Poorly formed command.  Try again.\n");
            continue;
        }
        else if (cmd->type == PUT) {
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
