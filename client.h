// Rowan MacLachlan
// rdm695 22265820
// CMPT 434 Derek Eager
// January 25th, 6pm

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

int get(char *local_file_name, char *remote_file_name);

int put(char *local_file_name, char *remote_file_name);

void quit(void);
