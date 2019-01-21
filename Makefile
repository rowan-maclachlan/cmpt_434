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
TARGET = server client beej_s beej_c
########################################
# Directories
OBJ = ./obj/
INC = ./include/
SRC = ./src/
$(shell mkdir -p $(OBJ))
########################################
# Compiler and linker options
CC = gcc
AR_OPTIONS = cr
C_FLAGS = -Wall -pedantic -g -I$(INC)
########################################

# Filename macroes
# server 
SERVER_H = $(INC)server.h
SERVER_OBJ = $(OBJ)server.o 
CLIENT_H = $(INC)client.h
CLIENT_OBJ = $(OBJ)client.o 
COMMON_H = $(INC)common.h
COMMON_OBJ = $(OBJ)common.o
TEST_OBJ = $(OBJ)test_server.o
BEEJ_H = 
BEEJ_S_OBJ = $(OBJ)beej_s.o
BEEJ_C_OBJ = $(OBJ)beej_c.o
# all
ALL_OBJ = $(CLIENT_OBJ) $(SERVER_OBJ) $(COMMON_OBJ) $(TEST_OBJ)
ALL_H = $(CLIENT_H) $(SERVER_H) $(COMMON_H)
EXEC = server client beej_s beej_c
########################################
# Recipes
.PHONY: server all clean

all: $(TARGET)

# SERVER 
server : $(SERVER_OBJ)
	$(CC) $^ -o $@ 
# CLIENT
client : $(CLIENT_OBJ)
	$(CC) $^ -o $@ 
# BEEJ SERVER
beej_s : $(BEEJ_S_OBJ)
	$(CC) $^ -o $@ 
# BEEJ CLIENT 
beej_c : $(BEEJ_C_OBJ)
	$(CC) $^ -o $@
# TEST 
test : $(TEST_OBJ)
	$(CC) $^ -o $@ 

# SERVER OBJ FILES
$(SERVER_OBJ) : $(SRC)server.c $(SERVER_H) $(COMMON_H)
	$(CC) $(C_FLAGS) -c $< -o $@
# CLIENT OBJ FILES
$(CLIENT_OBJ) : $(SRC)client.c $(CLIENT_H) $(COMMON_H)
	$(CC) $(C_FLAGS) -c $< -o $@
# BEEJ OBJ FILES
$(BEEJ_S_OBJ) : $(SRC)beej_s.c $(BEEJ_H)
	$(CC) $(C_FLAGS) -c $< -o $@
$(BEEJ_C_OBJ) : $(SRC)beej_c.c $(BEEJ_H)
	$(CC) $(C_FLAGS) -c $< -o $@
# TEST OBJ FILES
$(TEST_OBJ) : $(SRC)test.c $(ALL_H) 
	$(CC) $(C_FLAGS) -c $< -o $@
clean:
	rm -f $(ALL_OBJ) 
	rmdir $(OBJ)
	rm -f $(EXEC) test
