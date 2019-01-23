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

#define PORT "3490"
#define BACKLOG 10
#define HOSTNAME_LEN 256

struct addrinfo * find_server(int *sock_fd, struct addrinfo *servinfo) {
    int yes = 1;
    struct addrinfo *p;

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((*sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) { 
            perror("server: socket");
            continue;
        }
        printf("socket num: %d\n", *sock_fd);
        if (setsockopt(*sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("server: setsockopt");
            exit(1);
        }

        if (bind(*sock_fd, p->ai_addr, p->ai_addrlen) != -1) {
            break;
        } 
        else {
            close(*sock_fd);
            perror("server: bind");
        }
    }

    free(servinfo);

    return p;
}

int _get(int sockfd, struct command *cmd) {
    return GET;
}

int _put(int sockfd, struct command *cmd) {
    char buf[MAX_DATA_SIZE];
    int remaining_bytes = cmd->fsz;
    int num_bytes = 0;
    FILE *file;

    file = fopen(cmd->dest, "w");
    if (NULL == file) {
        fprintf(stderr, "Failed to open file %s.\n", cmd->dest);
        return -1;
    }

    while (remaining_bytes > 0) {
        if ((num_bytes = recv(sockfd, buf, MAX_DATA_SIZE-1, 0)) == -1) {
            perror("recv");
            return -1;
        }
        if (num_bytes != fwrite(buf, 1, num_bytes, file)) {
            fprintf(stderr, "Error writing file.\n");
        }
        remaining_bytes -= num_bytes;
    }

    fclose(file);
    return 0;
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
    char buf[MAX_DATA_SIZE];
    int sock_fd = 0; // listen on this fd
    int new_fd = 0; // listening for the child process
    int status;
    struct command *cmd;
    int yes = 1;
    int num_bytes = 0;
    char hostname[HOSTNAME_LEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}

    gethostname(hostname, HOSTNAME_LEN);
    printf("Running on host %s and port %s.\n", hostname, PORT);

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sock_fd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1); }

		if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock_fd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
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

        if ((num_bytes = recv(new_fd, buf, MAX_DATA_SIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[num_bytes] = '\0';
        printf("server: received '%s'\n", buf);

        if (NULL == (cmd = deserialize_cmd(buf))) {
            fprintf(stderr, "Failed to deserialize the command.\n");
            continue;
        }

        print_cmd(cmd);

        if (cmd->type == PUT) {
            _put(sock_fd, cmd);
        }
        else if (cmd->type == GET) {
            _get(sock_fd, cmd);
        }
        else {
           fprintf(stderr, "Invalid command format.\n");
        }

        free(cmd);

    }

    return 0;
}
