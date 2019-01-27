// Rowan MacLachlan
// rdm695 22265820
// CMPT 434 Derek Eager
// January 25th, 6pm

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include "tcp_common.h"

#define TOKENS 3

enum cmd_type _get_type(char *cmd) {
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

char *_from_type(enum cmd_type cmd_type) {
    if (QUIT == cmd_type) {
        return "quit";
    }
    else if (GET == cmd_type) {
        return "get";
    }
    else if (PUT == cmd_type) {
        return "put";
    }
    else {
        return "inv";
    }
}

struct command * _deserialize_cmd(char *cmd_buf) {
    char type[MAX_FILENAME_LEN] = { '\0' };
    char src[MAX_FILENAME_LEN] = { '\0' };
    char dest[MAX_FILENAME_LEN] = { '\0' };
    size_t size = 0;
    enum error err = 0;
    struct command *cmd = NULL;

    cmd = malloc(sizeof *cmd);
    if (NULL == cmd) {
        perror("malloc");
        return NULL;
    }
    if (5 != sscanf(cmd_buf, " %s %s %s %zu %u ", type, src, dest, &size, &err)) {
        fprintf(stderr, "sscanf failed to scan input.\n");
        free(cmd);
        return NULL;
    }

    cmd->type = _get_type(type);
    cmd->src = strdup(src);
    cmd->dest = strdup(dest);
    cmd->fsz = size;
    cmd->err = err;

    return cmd;
}

int _recv_file(char *buf, int sockfd) {
    size_t num_bytes = 0;

    if ((num_bytes = recv(sockfd, buf, FILE_BUFF_MAX-1, 0)) == -1) {
        perror("recv");
        return -1;
    }

    return num_bytes;
}

int _write_file(char *buf, FILE *file_to_write, size_t n_write) {
    size_t n_written = 0;

    if (n_write != (n_written = fwrite(buf, 1, n_write, file_to_write))) {
        perror("fwrite");
    }

    return n_written;
}

int _serialize_cmd(char *buf, struct command *cmd) {
    return sprintf(buf, "%s %s %s %zu %u%c",
            _from_type(cmd->type), cmd->src, cmd->dest, cmd->fsz, cmd->err, '\0');
}

int recv_cmd(int sockfd, struct command **cmd) {
    char cmd_buf[CMD_LIMIT] = { '\0' };

    if (-1 == (recv(sockfd, cmd_buf, CMD_LIMIT, 0))) {
        perror("recv");
        return -1;
    }

    printf("[%d] received serialized command '%s' on socket %d\n", getpid(), cmd_buf, sockfd);

    if (NULL == (*cmd = _deserialize_cmd(cmd_buf))) {
        fprintf(stderr, "Error: Process %d failed to deserialize the command.\n", getpid());
        return -1;
    }

    return 0;
}

void send_cmd(int sockfd, struct command *cmd) {
    char cmd_buf[CMD_LIMIT] = { '\0' };
    _serialize_cmd(cmd_buf, cmd);
    printf("[%d] sent serialized command '%s' on socket %d\n", getpid(), cmd_buf, sockfd);
    send(sockfd, cmd_buf, strlen(cmd_buf), 0);
}

size_t send_file(FILE *file, int sockfd, size_t n_bytes) {
    size_t n_read = 0;
    size_t n_send = 0;
    char file_buf[FILE_BUFF_MAX];

    printf("[%d] Sending file contents on socket %d.\n", getpid(), sockfd);
    while ((n_read = fread(file_buf, 1, FILE_BUFF_MAX, file)) > 0) {
        // Here n_read is the number of bytes of the file we have read.
        // We know already that n_bytes is a valid total file size
        // NOw, we have to send these bytes.  What do we do if send cannot send
        // 'n_read' bytes at a time?
        if(n_read != (n_send = send(sockfd, file_buf, n_read, 0))) {
            perror("send");
        }
        n_bytes -= n_send;
    }

    // If we never had any problems, n_bytes should be 0 here.
    return n_bytes;
}

size_t recv_write_file(int sockfd, FILE *file, size_t n_remaining) {
    size_t n_recvd = 0;
    size_t n_write = 0;
    char file_buf[FILE_BUFF_MAX];
    printf("[%d] Receiving file contents on socket %d.\n", getpid(), sockfd);
    while(n_remaining > 0) {
        n_recvd = _recv_file(file_buf, sockfd);
        n_write = _write_file(file_buf, file, n_recvd);
        n_remaining -= n_write;
    }

    return n_remaining;
}

int _proxy_alter_buf(char *file_buf, int n_recvd) {
    int buf_i = 0;
    int file_buf_i = 0;
    char buf[FILESIZE_MAX] = { 0 };
    // Copy the original buffer
    memcpy(buf, file_buf, n_recvd);
    
    for (buf_i = 0; buf_i < n_recvd; buf_i++) {
        char c = buf[buf_i]; // original character
        if (c == 'c' || c == 'm' || c == 'p' || c == 't') { // double char
            file_buf[file_buf_i] = c; //     
            file_buf[file_buf_i+1] = c;
            file_buf_i++;
        }
        else {
            file_buf[file_buf_i] = c;
        }
        file_buf_i++;
    }
    return file_buf_i;
}

// This function needs to return the amount of bytes ACTUALLY written to the
// file.  This will be the new file length to forward to the server.  Put this
// value into the n_remaining field - it is the address of the cmd filesize.
// Nevermind.  This will accomplished when we read the file back in preparation
// of sending it to the server.
size_t proxy_recv_write_file(int sockfd, FILE *file, size_t n_remaining) {
    size_t n_recvd = 0;
    size_t n_added = 0;
    size_t n_write = 0;
    char file_buf[2*FILE_BUFF_MAX] = { 0 };
    printf("[%d] Receiving file contents on socket %d.\n", getpid(), sockfd);
    while(n_remaining > 0) {
        // receive the file contents into the buffer
        n_recvd = _recv_file(file_buf, sockfd);
        // alter the buffer
        n_added = _proxy_alter_buf(file_buf, n_recvd);
        // write the altered buffer to file
        n_write = _write_file(file_buf, file, n_added);
        // we need to reduce n_remaining only by the bytes ACTUALLY recieved
        // from the client, because we are writing more than that to file.
        n_remaining -= n_recvd;
    }

    return n_remaining;
}

void free_cmd(struct command *cmd) {
    free(cmd->src);
    free(cmd->dest);
    free(cmd);
}

char * get_input(char *buf) {
    memset(buf, '\0', CMD_LIMIT);

    // get input
    if (NULL == fgets(buf, CMD_LIMIT, stdin)) {
        fprintf(stderr, "fgets failed.\n");
        return NULL;
    }
    // remove newline
    int newline_pos = strcspn(buf, "\n");
    buf[newline_pos] = '\0';

    return buf;
}

struct command * parse_cmd(char *buf) {
    struct command *cmd = NULL;
    char type[MAX_FILENAME_LEN] = { '\0' };
    char src[MAX_FILENAME_LEN] = { '\0' };
    char dest[MAX_FILENAME_LEN] = {'\0' };
    int toks = 0;

    if (NULL == buf) {
        fprintf(stderr, "Provided buffer is NULL, exiting...\n");
        return NULL;
    }

    cmd = malloc(sizeof *cmd);
    if (NULL == cmd) {
        fprintf(stderr, "malloc failed, exiting.\n");
        return NULL;
    }

    toks = sscanf(buf, " %s %s %s ", type, src, dest);
    if (1 == toks) { // Quit
        cmd->src = strdup("0");
        cmd->dest = strdup("0");
    }
    else if (3 != toks) { // Invalid
        fprintf(stderr, "sscanf failed to scan input.\n");
        return NULL;
    }
    else { // Fully formed
        cmd->src = strdup(src);
        cmd->dest = strdup(dest);
    }
    cmd->type = _get_type(type);
    cmd->fsz = 0;
    cmd->err = FILE_OK;

    return cmd;
}

void print_cmd(struct command *cmd) {
    if (NULL == cmd) {
        printf("NULL command.");
    }
    else if (cmd->src == NULL) {
        printf("command src is NULL");
    }
    else if (cmd->dest == NULL) {
        printf("command dest is NULL");
    }
    else {
        printf("type: %d, source: %s, dest: %s, size: %zu, err: %u\n",
                cmd->type, cmd->src, cmd->dest, cmd->fsz, cmd->err);
    }
}

void set_timeout(int sockfd) {
    struct timeval tv;
    // Set timeout
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}

/**
 * From Beej's Guide
 * get sockaddr, IPv4 or IPv6:
 */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) { // IPv4
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr); // IPv6
}
