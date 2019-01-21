// Rowan MacLachlan
// rdm695 22265820
// CMPT 434 Derek Eager
// January 25th, 6pm

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <port number>", argv[0]);
  }

  return 0;
}
