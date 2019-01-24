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


int main(int argc, char **argv) {
    char buf[MAX_DATA_SIZE];
    struct command *cmd;
    char type[8];
    char src[64];
    char dest[64];
    char ser[CMD_LIMIT];
   
    while(1) {
        memset(type, 0, 8);
        memset(src, 0, 64);
        memset(dest, 0, 64);
        memset(buf, 0, MAX_DATA_SIZE);

        printf("Enter command of the form '%s':\n", USAGE);

        cmd = parse_cmd(get_input(buf));
        print_cmd(cmd);

        printf("\nClient sending command '%s' with length %lu\n", ser, strlen(ser));

        free(cmd);
    }
    
    exit(EXIT_SUCCESS);
}


