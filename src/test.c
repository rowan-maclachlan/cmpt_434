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

#define USAGE "[get|put|quit] filenamesource filenamedest"

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

int _recv_cmd(char *cmd_buf) {
    char type[MAX_FILENAME_LEN] = { '\0' };
    char src[MAX_FILENAME_LEN] = { '\0' };
    char dest[MAX_FILENAME_LEN] = { '\0' };
    size_t size = 0;
    enum error err = 0;
    struct command *cmd = NULL;

    printf("Process %d received serialized command '%s'\n", getpid(), cmd_buf);

    cmd = malloc(sizeof (struct command));
    if (NULL == cmd) {
        perror("malloc");
        return -1;
    }
    if (5 != sscanf(cmd_buf, " %s %s %s %zu %u ", type, src, dest, &size, &err)) {
        fprintf(stderr, "sscanf failed to scan input.\n");
        free(cmd);
        return -1;
    }

    printf("%s, %s, %s, %zu, %u\n", type, src, dest, size, err);

    cmd->type = get_type(type);
    cmd->src = strdup(src);
    cmd->dest = strdup(dest);
    cmd->fsz = size;
    cmd->err = err;

    print_cmd(cmd);

    free(cmd);

    return 0;
}

char * _get_input(char *buf) {
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

int main(int argc, char **argv) {
    char cmd_buf[CMD_LIMIT] = { '\0' };
   
    while(1) {

        printf("Enter command of the form '%s':\n", USAGE);

        _recv_cmd(_get_input(cmd_buf));
    }
    
    exit(EXIT_SUCCESS);
}


