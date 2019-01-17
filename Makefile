########################################
# Rowan MacLachlan
# rdm695
# 11165820
# January 15th 2019 
# CMPT 332 Makaroff
# Makefile
########################################
# Fancy variable debugging tool
print-%  : ; @echo $* = $($*)
########################################
# OS and architecture macros
CURR_OS := $(shell uname -s)
ARCH := $(shell uname -m)
MAC_OS="Darwin"
LINUX_OS="Linux"
########################################
TARGET = server client
########################################
# Compiler and linker options
CC = gcc
AR_OPTIONS = cr
C_FLAGS = -Wall -pedantic -g
########################################
# Filename macroes
# server 
SERVER_H = server.h
SERVER_OBJ = server.o 
CLIENT_H = client.h
CLIENT_OBJ = client.o 
COMMON_H = common.h
COMMON_OBJ = common.o
TEST_OBJ = test_server.o
# all
ALL_OBJ = $(CLIENT_OBJ) $(SERVER_OBJ) $(COMMON_OBJ) $(TEST_OBJ)
ALL_H = $(CLIENT_H) $(SERVER_H) $(COMMON_H)
EXEC = server client
########################################
# Recipes
.PHONY: server all clean

all: $(TARGET)

# SERVER 
server : $(SERVER_OBJ)
	$(CC) $(SERVER_OBJ) -o server 
client : $(CLIENT_OBJ)
	$(CC) $(CLIENT_OBJ) -o client 
# TEST 
test : $(TEST_OBJ)
	$(CC) $(TEST_OBJ) -o test
# SERVER OBJ FILES
server.o : server.c $(SERVER_H) $(COMMON_H)
	$(CC) $(C_FLAGS) -c server.c
# CLIENT OBJ FILES
client.o : client.c $(CLIENT_H) $(COMMON_H)
	$(CC) $(C_FLAGS) -c client.c
# TEST OBJ FILES
test.o : test.c $(ALL_H) 
	$(CC) $(C_FLAGS) -c test.c

clean:
	rm -f $(ALL_OBJ) 
	rm -f $(EXEC) test
