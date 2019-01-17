// Rowan MacLachlan
// rdm695 22265820
// CMPT 434 Derek Eager
// January 25th, 6pm

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

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
};

struct addrinfo * find_server(int *sock_fd, struct addrinfo *servinfo);

struct command * alloc_cmd(enum cmd_type type, char *src, char *dest);

enum cmd_type get_type(char *cmd);

struct command * parse_input();

void print_command(struct command *cmd);

int get(char *local_file_name, char *remote_file_name);

int put(char *local_file_name, char *remote_file_name);

void quit(void);

