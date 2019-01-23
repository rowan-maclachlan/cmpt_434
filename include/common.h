#ifndef COMMON_H
#define COMMON_H
// Rowan MacLachlan
// rdm695 22265820
// CMPT 434 Derek Eager
// January 25th, 6pm

#include <sys/types.h>

#define MAX_DATA_SIZE 1024 // max number of bytes we can get at once
#define CMD_LIMIT 256

enum cmd_type {
    QUIT,
    GET,
    PUT, 
    INV
};

struct command { 
    enum cmd_type type;
    char *src;
    char *dest;
    int fsz;
};

void free_cmd(struct command *cmd);

char * get_input(char *buf);

void print_cmd(struct command *cmd);

struct command * deserialize_cmd(char *buf);

int serialize_cmd(char *buf, struct command *cmd);

void * get_in_addr(struct sockaddr *sa);

#endif // COMMON_H
