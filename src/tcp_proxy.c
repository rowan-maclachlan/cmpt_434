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

#define PORT "3201"
#define BACKLOG 10
#define HOSTNAME_LEN 256
#define PORT_SIZE 16
#define HOSTNAME_SIZE 64
#define USAGE "[get|put|quit] filenamesource filenamedest"
#define TMP_NAME "tmp"

/**
 * Returns  -1 If there is an error with the request
 *          or the number of bytes we failed to receive and write successfully
 * The tricky part is adjusting this method so that the file contents are stored
 * with duplicated 'c' 'm' 'p' and 't' characters.  This should be done when
 * writing the file so that the actual file length is forwarded to the server
 * correctly.
 * We also have to postpone sending the confirmation of file transfer back to
 * the client until we know that the server has actually received the full new
 * file.
 */
int _server_put(int sockfd, struct command *cmd) {
    FILE *file;
    int n_remaining = 0;

    file = fopen(cmd->dest, "w");
    if (NULL == file) {
        perror("proxy: fopen(cmd->dest, \"w\")");
        cmd->err = FILE_CANT_WRITE;
        send_cmd(sockfd, cmd);
        return -1;
    }

    cmd->err = FILE_OK;
    send_cmd(sockfd, cmd);

    if (0 != (n_remaining = proxy_recv_write_file(sockfd, file, cmd->fsz))) {
        fprintf(stderr, "Error: proxy: failed to receive/write full file file.\n");
    }

    fclose(file);

    // DON'T SEND CONFIRMATION BACK TO THE CLIENT YET.  We will not do this
    // until the actual server has given us ITS confirmation, and then we will
    // forward this confirmation.

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
        perror("proxy: fopen(cmd->src, \"r\")");
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
    sleep(1);
    n_remaining = send_file(file, sockfd, cmd->fsz);

    fclose(file);

    // Recieve confirmation from client.
    if (-1 == recv_cmd(sockfd, cmd)) {
        fprintf(stderr, "Error: proxy: failed to receive confirmation command.\n");
        return -1;
    }
    
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
        cmd->err = FILE_OVERSIZE;
        return -1;
    }

    // Send put request
    send_cmd(sockfd, cmd);
    // Recieve handshake
    if (-1 == recv_cmd(sockfd, cmd)) {
        fprintf(stderr, "Error: proxy: failed to receive handshake command.\n");
        cmd->err = CMD_FAILED;
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
    if (-1 == recv_cmd(sockfd, cmd)) {
        fprintf(stderr, "Error: proxy: failed to receive handshake command.\n");
        fclose(file);
        return -1;
    }

    if (FILE_OK != cmd->err) {
        fprintf(stderr, "Error: proxy: request: %d.\n", cmd->err);
        fclose(file);
        return -1;
    }

    // Receive file contents
    if (0 != (n_remaining = proxy_recv_write_file(sockfd, file, cmd->fsz))) {
        fprintf(stderr, "Error: proxy: failed to receive/write file.\n");
    }

    fclose(file);

    // Don't send the server his confirmation until we know the client has
    // successfully 'gotten' the file they requested.
    
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
            perror("proxy: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("proxy: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "proxy: failed to connect\n");
        return -1;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("proxy: connecting to %s on socket %d\n", s, sockfd);

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
		perror("proxy: getaddrinfo");
		return -1;
	}

    gethostname(hostname, HOSTNAME_LEN);
    printf("Running on host %s and port %s.\n", hostname, PORT);

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sock_fd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("proxy: socket");
			continue;
		}

		if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("proxy: setsockopt");
			return -1;
        }

		if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock_fd);
			perror("proxy: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "proxy: failed to bind to valid addrinfo");
        return -1;
    }

    if (listen(sock_fd, BACKLOG) == -1) {
        perror("proxy: listen");
        return -1;
    }

    printf("proxy: waiting for connections...\n");

    sin_size = sizeof their_addr;
    new_fd = accept(sock_fd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        perror("proxy: accept");
        return -1;
    }

    void * in_addr = get_in_addr((struct sockaddr *)&their_addr);
    inet_ntop(their_addr.ss_family, in_addr, s, sizeof s);

    printf("proxy: got connection from %s on socket %d\n", s, new_fd);

    return new_fd;
}

int _put(int client_sock, int server_sock, struct command *cmd) {
    char tmp_filename[MAX_FILENAME_LEN+1] = { 0 };
    int status = 0;
    // We can't give it the same filename, because the proxy may not
    // have the same directory structure of permissions as the actual
    // server.  We must give it a shorter, default name.  Not
    // "cmd->dest"
    strncpy(tmp_filename, cmd->dest, MAX_FILENAME_LEN);
    strncpy(cmd->dest, TMP_NAME, MAX_FILENAME_LEN);
    if (0 != (status = _server_put(client_sock, cmd))) {
        fprintf(stderr, "Error: proxy: failed to execute put command with client: %d\n", status);
    }
    // restore the original 'cmd->dest' filename
    strncpy(cmd->dest, tmp_filename, MAX_FILENAME_LEN);
    // Forward the put command to the server now, copying our temporary
    // file and sending it.
    strncpy(tmp_filename, cmd->src, MAX_FILENAME_LEN);
    strncpy(cmd->src, TMP_NAME, MAX_FILENAME_LEN);
    if (0 != (status = _client_put(server_sock, cmd))) {
        fprintf(stderr, "Error: proxy: failed to execute put command with server: %d\n", status);
    }

    // forward confirmation message to the waiting client.
    strncpy(cmd->src, tmp_filename, MAX_FILENAME_LEN);
    send_cmd(client_sock, cmd);

    return 0;
}

int _get(int client_sock, int server_sock, struct command *cmd) {
    char tmp_filename[MAX_FILENAME_LEN] = { 0 };
    int status = 0;
    // This may be significantly different from the approach for 'put'.
    // The get command needs to be forwarded to the server FIRST, before
    // anything about files is done.  Just forward the command, but we
    // need to change the destination to a temporary name, and change it
    // back when we forward the actual file back to the client.  We will
    // need to change the source at that point, too.
    strncpy(tmp_filename, cmd->dest, MAX_FILENAME_LEN);
    strncpy(cmd->dest, TMP_NAME, MAX_FILENAME_LEN);
    // Forward the get command to the server
    if (0 != (status = _client_get(server_sock, cmd))) {
        fprintf(stderr, "Error: proxy: failed to execute get command with server: %d\n", status);
    }
    // restore the original 'cmd->dest' filename
    strncpy(cmd->dest, tmp_filename, MAX_FILENAME_LEN);
    // Respond with to the client with the file content we previously
    // acquired.  First with the handshake, then the file contents. 
    // Finally, recieve confirmation of file write, and forward to
    // server.
    strncpy(tmp_filename, cmd->src, MAX_FILENAME_LEN);
    strncpy(cmd->src, TMP_NAME, MAX_FILENAME_LEN);
    if (0 != _server_get(client_sock, cmd)) {
        fprintf(stderr, "Error: proxy: failed to execute get command with client: %d\n", status);
    }

    // forward the confirmation from the client to the server.
    strncpy(cmd->src, tmp_filename, MAX_FILENAME_LEN);
    send_cmd(server_sock, cmd);

    return 0;
}

int main(int argc, char **argv) {
    char port[PORT_SIZE] = { 0 };
    char hostname[HOSTNAME_SIZE] = { 0 };
    int client_sock = 0;
    int server_sock = 0;
    struct command cmd;

    if (argc != 3) {
        printf("Usage: %s <host name> <port number>", argv[0]);
        exit(1);
    }
    else {
        strncpy(hostname, argv[1], strlen(argv[1]));
        hostname[strlen(argv[1])] = '\0';
        strncpy(port, argv[2], strlen(argv[2]));
        port[strlen(argv[2])] = '\0';
        printf("Connecting to hostname %s on port %s\n", hostname, port);
    }

    if (0 >= (server_sock = _client_connect(hostname, port))) {
        fprintf(stderr, "Failed to connect to server.\n");
        exit(EXIT_FAILURE);
    }

    // recv send loop
    while(1) {
        if (0 >= (client_sock = _server_connect())) {
            fprintf(stderr, "Failed to connect to clients.\n");
            exit(EXIT_FAILURE);
        }

        while(1) {
            printf("proxy: waiting for commands...\n");

            if (-1 == recv_cmd(client_sock, &cmd)) {
                fprintf(stderr, "Error: proxy: failed to receive command.\n");
                continue;
            }

            if (cmd.type == PUT) {
                _put(client_sock, server_sock, &cmd);
            }
            else if (cmd.type == GET) {
                _get(client_sock, server_sock, &cmd);
            }
            else if (cmd.type == QUIT) {
                printf("Proxy: closing connections with socket %d.\n", client_sock);
                break;
            }
            else {
                fprintf(stderr, "Invalid command option.  Try again.\n");
            }
        }
        close(client_sock);
    }

    close(server_sock);

    exit(EXIT_SUCCESS);
}

