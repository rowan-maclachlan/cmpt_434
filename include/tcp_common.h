#ifndef COMMON_H
#define COMMON_H
// Rowan MacLachlan
// rdm695 22265820
// CMPT 434 Derek Eager
// January 25th, 6pm

#include <sys/types.h>

#define CMD_SIZE (sizeof (struct command) + 4)
#define TIMEOUT 10
#define MAX_FILENAME_LEN 64
#define FILE_BUFF_MAX 1024
#define FILESIZE_MAX 1024*FILE_BUFF_MAX

enum cmd_type {
    QUIT = 0,
    GET = 1,
    PUT = 2, 
    INV = 3
};

enum error {
    FILE_OK = 0,
    FILE_OVERSIZE = 1,
    FILE_EMPTY = 2,
    FILE_CANT_READ = 3,
    FILE_CANT_WRITE = 4,
    FILE_INCOMPLETE = 5,
    CMD_FAILED = 6
};

struct command { 
    enum cmd_type type;
    char src[MAX_FILENAME_LEN+1];
    char dest[MAX_FILENAME_LEN+1];
    size_t fsz;
    enum error err;
};

size_t proxy_recv_write_file(int sockfd, FILE *file, size_t n_remaining);

size_t recv_write_file(int sockfd, FILE *file, size_t n_remaining);

int recv_cmd(int sockfd, struct command *cmd);

void send_cmd(int sockfd, struct command *cmd);

size_t send_file(FILE *file, int sockfd, size_t n_bytes );

char *err_to_str(enum error err);

void free_cmd(struct command *cmd);

char * get_input(char *buf);

void print_cmd(struct command *cmd);

int parse_cmd(char *buf, struct command *cmd);

void set_timeout(int sockfd);

void * get_in_addr(struct sockaddr *sa);

#endif // COMMON_H
