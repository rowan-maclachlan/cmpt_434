#ifndef SERVER_H
#define SERVER_H
// Rowan MacLachlan
// rdm695 22265820
// CMPT 434 Derek Eager
// January 25th, 6pm

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// struct addrinfo * find_server(int *sock_fd, struct addrinfo *servinfo);

int get(char *local_file_name, char *remote_file_name);

int put(char *local_file_name, char *remote_file_name);

void quit(void);

#endif // SERVER_H
