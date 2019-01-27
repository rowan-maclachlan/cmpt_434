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

#include "tcp_server.h"
#include "tcp_common.h"

#define PORT "3200"
#define BACKLOG 10
#define HOSTNAME_LEN 256

/**
 * Returns  -1 If there is an error with the request
 *          or the number of bytes we failed to receive and write successfully
 */
int _put(int sockfd, struct command *cmd) {
    FILE *file;
    int n_remaining = 0;

    file = fopen(cmd->dest, "w");
    if (NULL == file) {
        perror("server: fopen(cmd->dest, \"w\")");
        cmd->err = FILE_CANT_WRITE;
        send_cmd(sockfd, cmd);
        return -1;
    }

    // Send handshake
    cmd->err = FILE_OK;
    send_cmd(sockfd, cmd);

    // receive file
    if (0 != (n_remaining = recv_write_file(sockfd, file, cmd->fsz))) {
        fprintf(stderr, "Error: server: failed to receive/write full file.\n");
        cmd->err = FILE_INCOMPLETE;
        cmd->fsz = n_remaining;
    }

    fclose(file);

    // Send confirmation command.
    send_cmd(sockfd, cmd);

    return n_remaining;
}

/**
 * Returns  -1 If there is an error with the request
 *          or the number of bytes we failed to receive and write successfully
 */
int _get(int sockfd, struct command *cmd) {
    FILE *file;
    int n_remaining = 0;

    file = fopen(cmd->src, "r");
    if (NULL == file) {
        perror("server: fopen(cmd->src, \"r\")");
        cmd->err = FILE_CANT_READ;
        send_cmd(sockfd, cmd);
        return -1;
    }

    fseek(file, 0L, SEEK_END);
    cmd->fsz = ftell(file);
    fseek(file, 0L, SEEK_SET);
    if (cmd->fsz > FILESIZE_MAX) {
        fprintf(stderr, "Filesize exceeds limits.\n");
        cmd->err = FILE_OVERSIZE;
        send_cmd(sockfd, cmd);
        fclose(file);
        return -1;
    }
    else if (cmd->fsz <= 0) {
        fprintf(stderr, "File is empty.\n");
        cmd->err = FILE_EMPTY;
        send_cmd(sockfd, cmd);
        fclose(file);
        return -1;
    }

    // Send handshake message
    cmd->err = FILE_OK;
    send_cmd(sockfd, cmd);
    sleep(1);
    // send file
    n_remaining = send_file(file, sockfd, cmd->fsz);
    fclose(file);

    // Recieve confirmation that the file was fully written successfully.
    if (-1 == recv_cmd(sockfd, cmd)) {
        fprintf(stderr, "Error: server: failed to receive the confirmation command.\n");
        return -1;
    }

    return n_remaining;
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
    struct command cmd;
    int yes = 1;
    char hostname[HOSTNAME_LEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		perror("server: getaddrinfo");
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

        printf("server: got connection from %s on socket %d\n", s, new_fd);

        while(1) {
            printf("server: waiting for commands...\n");

            if (-1 == recv_cmd(new_fd, &cmd)) {
                fprintf(stderr, "Error: server: failed to receive command.\n");
                sleep(2);
                continue;
            }

            if (cmd.type == PUT) {
                if (0 != (status = _put(new_fd, &cmd))) {
                    fprintf(stderr, "Error: server: failed to execute put command: %d\n", status);
                }
            }
            else if (cmd.type == GET) {
                if (0 != (status = _get(new_fd, &cmd))) {
                    fprintf(stderr, "Error: server: failed to execute get command: %d\n", status);
                }
            }
            else if (cmd.type == QUIT) {
                printf("server: closing connections with socket %d.\n", new_fd);
                close(new_fd);
                break;
            }
            else {
               fprintf(stderr, "Invalid command format.\n");
            }
        }
    }

    return 0;
}
