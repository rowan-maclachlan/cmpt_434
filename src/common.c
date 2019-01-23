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

void free_cmd(struct command *cmd) {
    free(cmd->src);
    free(cmd->dest);
    free(cmd);
}

char * get_input(char *buf) {
    memset(buf, 0, CMD_LIMIT); 

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

struct command * deserialize_cmd(char *buf) {
    struct command *cmd = NULL;
    char type[MAX_FILENAME_LEN] = { 0 };
    char src[MAX_FILENAME_LEN] = { 0 };
    char dest[MAX_FILENAME_LEN] = { 0 };

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

    return cmd;
}

int serialize_cmd(char *buf, struct command *cmd) {
    return sprintf(buf, "%s %s %s %d%c", _from_type(cmd->type), cmd->src, cmd->dest, cmd->fsz, '\0');
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
        printf("type: %d, source: %s, dest: %s, size: %d\n", cmd->type, cmd->src, cmd->dest, cmd->fsz);
    }
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
