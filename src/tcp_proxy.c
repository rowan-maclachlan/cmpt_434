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
#define PORT_SIZE 16
#define HOSTNAME_SIZE 64
#define USAGE "[get|put|quit] filenamesource filenamedest"

/**
 * Returns  -1 If there is an error with the request
 *          or the number of bytes we failed to receive and write successfully
 */
int _server_put(int sockfd, struct command *cmd) {
    FILE *file;
    int n_remaining = 0;

    file = fopen(cmd->dest, "w");
    if (NULL == file) {
        perror("server: fopen(cmd->dest, \"w\")");
        cmd->err = FILE_CANT_WRITE;
        send_cmd(sockfd, cmd);
        return -1;
    }

    cmd->err = FILE_OK;
    send_cmd(sockfd, cmd);

    if (0 != (n_remaining = recv_write_file(sockfd, file, cmd->fsz))) {
        fprintf(stderr, "Error: server: failed to receive/write full file file.\n");
    }

    fclose(file);

    return n_remaining;
}

/**
 * Returns  -1 If there is an error with the request
 *          or the number of bytes we failed to receive and write successfully
 */
int _server_get(int sockfd, struct command *cmd) {
    FILE *file;
    size_t n_remaining = 0;

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

    cmd->err = FILE_OK;
    send_cmd(sockfd, cmd);
    n_remaining = send_file(file, sockfd, cmd->fsz);

    fclose(file);

    return n_remaining;
}


int _client_put(int sockfd, struct command *cmd) {
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
    if (cmd->fsz > FILESIZE_MAX) {
        fprintf(stderr, "Filesize of %zu bytes exceeds limits.\n", cmd->fsz);
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

int _client_get(int sockfd, struct command *cmd) {
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

int _client_connect(char *hostname, char *port) {
    int sockfd;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *p;
    char s[INET6_ADDRSTRLEN];
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
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
        return -1;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    return sockfd;
}

/**
 * From Beej's Guide to Network Programming.
 */
int _server_connect() {
    char s[INET6_ADDRSTRLEN];
    struct sockaddr_storage their_addr; // connector's address information
    struct addrinfo hints;
    socklen_t sin_size;
    struct addrinfo *servinfo;
    struct addrinfo *p;
    int sock_fd = 0; // listen on this fd
    int new_fd = 0; // listening for the child process
    int status;
    int yes = 1;
    char hostname[HOSTNAME_LEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		perror("server: getaddrinfo");
		return -1;
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
			return -1;
        }

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
        return -1;
    }

    if (listen(sock_fd, BACKLOG) == -1) {
        perror("listen");
        return -1;
    }

    printf("server: waiting for connections...\n");

    sin_size = sizeof their_addr;
    new_fd = accept(sock_fd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        return -1;
    }

    void * in_addr = get_in_addr((struct sockaddr *)&their_addr);
    inet_ntop(their_addr.ss_family, in_addr, s, sizeof s);

    printf("server: got connection from %s...\n", s);

    return new_fd;
}


int main(int argc, char **argv) {
    char port[PORT_SIZE] = { 0 };
    char hostname[HOSTNAME_SIZE] = { 0 };
    int client_sock = 0;
    int server_sock = 0;
    char file_buf[2*FILESIZE_MAX] = { 0 };
    struct command *cmd = NULL;

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

    if (0 >= (server_sock = _server_connect())) {
        fprintf(stderr, "Failed to connect to server.\n");
        exit(EXIT_FAILURE);
    }

    if (0 >= (client_sock = _client_connect(hostname, port))) {
        fprintf(stderr, "Failed to connect to any client.\n");
        exit(EXIT_FAILURE);
    }

    // recv send loop
    while(1) {
        printf("server: waiting for commands...\n");

        if (-1 == recv_cmd(client_sock, &cmd)) {
            fprintf(stderr, "Error: server: failed to receive command.\n");
            break;
        }

        if (cmd->type == PUT) {
            if (0 != _client_put(server_sock, cmd)) {
                fprintf(stderr, "Error: server: failed to execute put command.\n");
            }
            if (0 != _server_put(client_sock, cmd)) {
                fprintf(stderr, "Error: client: failed to execute put command.\n");
            }
        }
        else if (cmd->type == GET) {
            if (0 != _client_get(server_sock, cmd)) {
                fprintf(stderr, "Error: server: failed to execute put command.\n");
            }
            if (0 != _server_get(client_sock, cmd)) {
                fprintf(stderr, "Error: client: failed to execute put command.\n");
            }
        }
        else {
            fprintf(stderr, "Invalid command option.  Try again.\n");
        }

        free(cmd);
    }

    close(client_sock);
    close(server_sock);

    exit(EXIT_SUCCESS);
}

