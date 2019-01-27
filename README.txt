Rowan Maclachlan
rdm695 11165820
Sunday January 27th 2019
CMPT 434 Eager

TESTING INSTRUCTIONS

 1) The executable files for the TCP server, client, and proxy server reside in the
folders 'server_dir', 'client_dir', and 'proxy_dir', respectively.
 2) The application can be tested by launching each process in order:
   Either with `./server` and then `./client <host> <port>`,
   or with `./server`, `proxy <host> <port>` and then `./client <host> <port>`.
 3) The <host> and <port> arguments you should provide are those which are given
   as output by the other process.  For example, if I run:
   `./server`
   I will get output like this:
   "server running with hostname My-MacBook-Air and port 32001"
   Then, I will launch the client like this:
   `./client My-MacBook-Air 32001` 
   and the client will connect to the server.
 4) Once the proxy and server are running (or just the regular server), the
 client is launched.  When the client is launched, the user is presented with a
 command prompt.  The commands should be input as described in the assignment
 specification.  In the "client_dir" folder, however, there is a file called
 "test.txt" which the user can provide as an argument to test the application.

POSSIBLE ISSUES WITH TESTING:

 - It is possible that on the machine you are testing, the default ports used by
   the server and the proxy server will not be available.  If this is the case,
   you can alter the `#define` statements at the top of `src/tcp_proxy.c` and
   `src/tcp_server.c` to provide alternate port numbers for them to listen on.
   Then, issue the `make` command to regenerate the executables.
 - If any material necessary for testing the application appears to be missing,
   try running `make` in the root directory of the assignment.
