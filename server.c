// Rowan MacLachlan
// rdm695 22265820
// CMPT 434 Derek Eager
// January 25th, 6pm

#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "server.h"
#include "common.h"

#define PORT "32000"
#define CMD_LIMIT 100
#define TOKENS 3
#define USAGE "[get|put|quit] filenamesource filenamedest"
#define MAX_FILENAME_LEN 30
#define BACKLOG 10

struct addrinfo * find_server(int *sock_fd, struct addrinfo *servinfo) {
    int yes = 1;
    struct addrinfo *p;

    for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((*sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) { 
        perror("server: socket");
        continue;
      }
      if (setsockopt(*sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("server: setsockopt");
        exit(1);
      }
      if (bind(*sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
        printf("\nsa_family, sa_data, p->ai_addrlen: %d, %s, %d\n",
                p->ai_addr->sa_family, p->ai_addr->sa_data, p->ai_addrlen);

        close(*sock_fd);
        perror("server: bind");
        continue;
      } 
    }
    free(servinfo);

    return p;
}

void free_cmd(struct command *cmd) {
    free(cmd->src);
    free(cmd->dest);
    free(cmd);
}

struct command * alloc_cmd(enum cmd_type type, char *src, char *dest) {
    struct command *new_cmd;

    new_cmd = malloc(sizeof new_cmd);
    new_cmd->src = strdup(src);
    free(src);
    new_cmd->dest = strdup(dest);
    free(dest);

    return new_cmd;
}

enum cmd_type get_type(char *cmd) {
    if (0 == strncmp("put", cmd, 3)) {
        return PUT;
    }
    else if (0 == strncmp("get", cmd, 3)) {
        return GET;
    }
    else if (0 == strncmp("quit", cmd, 3)) {
        return QUIT;
    } 
    else {
        return INV;
    }
}

struct command * parse_input() {
    enum cmd_type cmd;
    char **ap, *argv[TOKENS], *inputstring;
    char line[CMD_LIMIT];

    memset(line, 0, CMD_LIMIT); 
    if (NULL == fgets(line, CMD_LIMIT, stdin)) {
        return NULL;   
    }

    for (ap = argv; (*ap = strsep(&inputstring, " \t")) != NULL;) {
        if (**ap != '\0') {
            if (++ap >= &argv[TOKENS]) {
                break;
            }
        }
    }

    printf("arg1: %s, arg2: %s, arg3: %s\n", argv[0], argv[1], argv[2]);

    if (INV == (cmd = get_type(argv[0]))) {
        printf("Invalid command type '%s'", argv[0]);
        return NULL;
    }
    if (strlen(argv[1]) < 1 || strlen(argv[1]) > MAX_FILENAME_LEN) {
        printf("Invalid source filename '%s'", argv[1]);
        return NULL;
    }
    if (strlen(argv[2]) < 1 || strlen(argv[2]) > MAX_FILENAME_LEN) {
        printf("Invalid destination filename '%s'", argv[2]);
        return NULL;
    }

    return alloc_cmd(cmd, argv[1], argv[2]);
}

void print_command(struct command *cmd) {
    if (NULL == cmd) {
        printf("NULL command.");
    }
    else {
        printf("type: %d, source: %s, dest: %s", cmd->type, cmd->src, cmd->dest);
    }
}

int get(char *local_file_name, char *remote_file_name) {
  return GET;
}

int put(char *local_file_name, char *remote_file_name) {
  return PUT;
}

void quit(void) {
  exit(EXIT_FAILURE);
}

/** 
 * From Beej's Guide to Network Programming.
 */
int main(int argc, char **argv) {
    char s[INET6_ADDRSTRLEN];
    struct sockaddr_storage their_addr; // connector's address information
    struct addrinfo hints;
    socklen_t sin_size;
    struct addrinfo *servinfo;
    struct addrinfo *p;
    int sock_fd = 0; // listen on this fd
    int new_fd = 0; // listening for the child process
    int status;
    struct command *cmd;
    char hostname[1024];

    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    if ((status = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    } 
    
    if ((p = find_server(&sock_fd, servinfo)) == NULL) {
        fprintf(stderr, "server: failed to bind to valid addrinfo");
        exit(1);
    }
    
    if (listen(sock_fd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    
    printf("server: waiting for connections...\n");
    
    while(1) {
        sin_size = sizeof their_addr; 
        new_fd = accept(sock_fd, (struct sockaddr *)&their_addr, &sin_size); 
        if (new_fd == -1) {
            perror("accept");
            continue;
        } 
        void * in_addr = get_in_addr((struct sockaddr *)&their_addr);
        inet_ntop(their_addr.ss_family, in_addr, s, sizeof s);
    
        printf("server: got connection from %s\n", s); 

        printf(USAGE);
        cmd = parse_input();
        while (NULL == (cmd = parse_input())) {
            printf(USAGE);
        }

        print_command(cmd);

        free(cmd);

        if (send(new_fd, "Hello, world!", 13, 0) == -1) {
            perror("send");
        }
        close(new_fd);

    }
    return 0;
}


