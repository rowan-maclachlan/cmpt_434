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

int _deserialize_cmd(char *cmd_buf, struct command *cmd) {

    memset(cmd->src, 0, MAX_FILENAME_LEN+1);
    memset(cmd->dest, 0, MAX_FILENAME_LEN+1);
    if (5 != sscanf(cmd_buf, "%u %s %s %zu %u",
                &(cmd->type), cmd->src, cmd->dest, &(cmd->fsz), &(cmd->err))) {
        fprintf(stderr, "sscanf failed to scan input.\n");
        return -1;
    }

    return 0;
}

int _recv_file(char *buf, int sockfd) {
    size_t num_bytes = 0;

    if ((num_bytes = recv(sockfd, buf, FILE_BUFF_MAX, 0)) == -1) {
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
    return sprintf(buf, "%u %s %s %zu %u",
            cmd->type, cmd->src, cmd->dest, cmd->fsz, cmd->err);
}

int recv_cmd(int sockfd, struct command *cmd) {
    char cmd_buf[CMD_SIZE] = { '\0' };

    if (-1 == (recv(sockfd, cmd_buf, CMD_SIZE, 0))) {
        perror("recv");
        return -1;
    }

    printf("[%d] received serialized command '%s' on socket %d\n", getpid(), cmd_buf, sockfd);

    if (-1 == (_deserialize_cmd(cmd_buf, cmd))) {
        fprintf(stderr, "Error: Process %d failed to deserialize the command.\n", getpid());
        return -1;
    }

    return 0;
}

void send_cmd(int sockfd, struct command *cmd) {
    char cmd_buf[CMD_SIZE] = { '\0' };
    _serialize_cmd(cmd_buf, cmd);
    printf("[%d] sent serialized command '%s' on socket %d\n", getpid(), cmd_buf, sockfd);
    send(sockfd, cmd_buf, CMD_SIZE, 0);
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
    char file_buf[FILE_BUFF_MAX];
    int orig = n_remaining;
    printf("[%d] Receiving file contents on socket %d.\n", getpid(), sockfd);
    while(orig > 0) {
        n_recvd = _recv_file(file_buf, sockfd);
        _write_file(file_buf, file, n_recvd);
        orig -= n_recvd;
        if (0 == n_recvd) {
            break;
        }
    }
    printf("[%d] Received all but %d bytes of content on socket %d.\n",
            getpid(), orig, sockfd);

    return orig;
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
    int orig = n_remaining;
    char file_buf[2*FILE_BUFF_MAX] = { 0 };
    printf("[%d] Receiving file contents on socket %d.\n", getpid(), sockfd);
    while(orig > 0) {
        // receive the file contents into the buffer
        n_recvd = _recv_file(file_buf, sockfd);
        // alter the buffer
        n_added = _proxy_alter_buf(file_buf, n_recvd);
        // write the altered buffer to file
        _write_file(file_buf, file, n_added);
        // we need to reduce n_remaining only by the bytes ACTUALLY recieved
        // from the client, because we are writing more than that to file.
        orig -= n_recvd;
        if (0 == n_recvd) {
            break;
        }
    }
    printf("[%d] Received all but %d bytes of content on socket %d.\n",
            getpid(), orig, sockfd);

    return orig;
}

char * get_input(char *buf) {
    memset(buf, '\0', CMD_SIZE);

    // get input
    if (NULL == fgets(buf, CMD_SIZE, stdin)) {
        fprintf(stderr, "fgets failed.\n");
        return NULL;
    }
    // remove newline
    int newline_pos = strcspn(buf, "\n");
    buf[newline_pos] = '\0';

    return buf;
}

int parse_cmd(char *buf, struct command *cmd) {
    char type[MAX_FILENAME_LEN] = { '\0' };
    int toks = 0;

    if (NULL == buf) {
        fprintf(stderr, "Provided buffer is NULL, exiting...\n");
        return -1;
    }

    if (NULL == cmd) {
        fprintf(stderr, "Provided command is NULL, exiting...\n");
        return -1;
    }

    toks = sscanf(buf, " %s %s %s ", type, cmd->src, cmd->dest);
    if (1 == toks) { // Quit
        memset(cmd->src, 0, MAX_FILENAME_LEN);
        memset(cmd->src, 0, MAX_FILENAME_LEN);
    }
    else if (3 == toks) { // Well formed
        cmd->src[MAX_FILENAME_LEN] = '\0';
        cmd->dest[MAX_FILENAME_LEN] = '\0';
    }
    else { // Invalid
        fprintf(stderr, "sscanf failed to scan input.\n");
        return -1;
    }

    cmd->type = _get_type(type);
    cmd->fsz = 0;
    cmd->err = FILE_OK;

    return 0;
}

void print_cmd(struct command *cmd) {
    if (NULL == cmd) {
        printf("NULL command.");
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
