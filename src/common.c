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

#include "common.h"

#define TOKENS 3
#define MAX_FILENAME_LEN 64
#define FILESIZE_MAX 1024

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

int _deserialize_cmd(char *cmd_buf, struct command **cmd) {
    char type[MAX_FILENAME_LEN] = { '\0' };
    char src[MAX_FILENAME_LEN] = { '\0' };
    char dest[MAX_FILENAME_LEN] = { '\0' };
    size_t size = 0;
    enum error err = 0;

    if (NULL == cmd) {
        fprintf(stderr, "NULL **cmd.\n");
        return -1;
    }
    *cmd = malloc(sizeof cmd);
    if (NULL == *cmd) {
        fprintf(stderr, "malloc failed.\n");
        return -1;
    }
    if (5 != sscanf(cmd_buf, "%s %s %s %zu %u", type, src, dest, &size, &err)) {
        fprintf(stderr, "sscanf failed to scan input.\n");
        free(*cmd);
        *cmd = NULL;
        return -1;
    }

    (*cmd)->type = _get_type(type);
    (*cmd)->src = strndup(src, strlen(src) + 1);
    (*cmd)->dest = strndup(dest, strlen(dest) + 1);
    (*cmd)->fsz = size;
    (*cmd)->err = err;

    if (NULL == *cmd ||
           0 == strcmp((*cmd)->src, "") ||
           0 == strcmp((*cmd)->dest, "")) {
        fprintf(stderr, "Error: Process %d failed to deserialize the command.\n", getpid());
        return -1;
    }

    return 0;
}

int _recv_file(char *buf, int sockfd) {
    size_t num_bytes = 0;

    if ((num_bytes = recv(sockfd, buf, MAX_DATA_SIZE-1, 0)) == -1) {
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

    if (recv(sockfd, cmd_buf, CMD_LIMIT, 0) == -1) {
        perror("recv");
        return -1;
    }

    printf("Process %d received serialized command '%s'\n", getpid(), cmd_buf);

    if (-1 == _deserialize_cmd(cmd_buf, cmd)) {
        fprintf(stderr, "Error: Process %d failed to deserialize the command.\n", getpid());
        return -1;
    }

    if (*cmd == NULL ||
            0 == strcmp((*cmd)->src, "") ||
            0 == strcmp((*cmd)->dest, "")) {
        fprintf(stderr, "Error: Process %d failed to deserialize the command.\n", getpid());
        return -1;
    }


    return 0;
}

void send_cmd(int sockfd, struct command *cmd) {
    char cmd_buf[CMD_LIMIT] = { '\0' };
    _serialize_cmd(cmd_buf, cmd);
    printf("Process %d sent serialized command '%s'\n", getpid(), cmd_buf);
    send(sockfd, cmd_buf, strlen(cmd_buf), 0);
}

size_t send_file(FILE *file, int sockfd, size_t n_bytes) {
    size_t n_read = 0;
    size_t n_send = 0;
    char file_buf[MAX_DATA_SIZE];

    while ((n_read = fread(file_buf, 1, sizeof file_buf, file)) > 0) {
        if(n_read != (n_send = send(sockfd, file_buf, n_read, 0))) {
            perror("send");
        }
        n_bytes -= n_send;
    }

    return n_bytes;
}

size_t recv_write_file(int sockfd, FILE *file, size_t n_remaining) {
    size_t n_recvd = 0;
    size_t n_write = 0;
    char file_buf[MAX_DATA_SIZE];
    while(n_remaining > 0) {
        n_recvd = _recv_file(file_buf, sockfd);
        n_write = _write_file(file_buf, file, n_recvd);
        n_remaining -= n_write;
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

    if (NULL == buf) {
        fprintf(stderr, "Provided buffer is NULL, exiting...\n");
        return NULL;
    }

    cmd = malloc(sizeof cmd);
    if (NULL == cmd) {
        fprintf(stderr, "malloc failed, exiting.\n");
        return NULL;
    }

    if (3 != sscanf(buf, "%s %s %s", type, src, dest)) {
        fprintf(stderr, "sscanf failed to scan input.\n");
        return NULL;
    }
    cmd->type = _get_type(type);
    cmd->src = strdup(src);
    cmd->dest = strdup(dest);
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
