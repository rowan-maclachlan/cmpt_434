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
TARGET = tcp_server tcp_client tcp_proxy beej_s beej_c test
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
C_FLAGS = -Wall -pedantic -g
INC_FLAGS = -I$(INC)
########################################

# Filename macroes
# server 
SERVER_H = $(INC)tcp_server.h
SERVER_OBJ = $(OBJ)tcp_server.o 
CLIENT_H = $(INC)tcp_client.h
CLIENT_OBJ = $(OBJ)tcp_client.o 
COMMON_H = $(INC)tcp_common.h
COMMON_OBJ = $(OBJ)tcp_common.o
TEST_OBJ = $(OBJ)test_server.o
PROXY_OBJ = $(OBJ)tcp_proxy.o
BEEJ_H = 
BEEJ_S_OBJ = $(OBJ)beej_s.o
BEEJ_C_OBJ = $(OBJ)beej_c.o
# all
ALL_OBJ = $(CLIENT_OBJ) $(SERVER_OBJ) $(COMMON_OBJ) $(TEST_OBJ) \
		  $(BEEJ_S_OBJ) $(BEEJ_C_OBJ) $(PROX_OBJ)
ALL_H = $(CLIENT_H) $(SERVER_H) $(COMMON_H)
EXEC = tcp_server tcp_client tcp_proxy beej_s beej_c
########################################
# Recipes
.PHONY: server all clean

all: $(TARGET)

# PROXY 
tcp_proxy : $(PROXY_OBJ) $(COMMON_OBJ)
	$(CC) $^ -o $@ 
# SERVER 
tcp_server : $(SERVER_OBJ) $(COMMON_OBJ)
	$(CC) $^ -o $@ 
# CLIENT
tcp_client : $(CLIENT_OBJ) $(COMMON_OBJ)
	$(CC) $^ -o $@ 
# BEEJ SERVER
beej_s : $(BEEJ_S_OBJ)
	$(CC) $^ -o $@ 
# BEEJ CLIENT 
beej_c : $(BEEJ_C_OBJ)
	$(CC) $^ -o $@
# TEST 
test : $(TEST_OBJ) $(COMMON_OBJ)
	$(CC) $^ -o $@ 

# PROXY OBJ FILES
$(PROXY_OBJ) : $(SRC)tcp_proxy.c $(COMMON_H)
	$(CC) $(INC_FLAGS) $(C_FLAGS) -c $< -o $@
# SERVER OBJ FILES
$(SERVER_OBJ) : $(SRC)tcp_server.c $(SERVER_H) $(COMMON_H)
	$(CC) $(INC_FLAGS) $(C_FLAGS) -c $< -o $@
# CLIENT OBJ FILES
$(CLIENT_OBJ) : $(SRC)tcp_client.c $(CLIENT_H) $(COMMON_H)
	$(CC) $(INC_FLAGS) $(C_FLAGS) -c $< -o $@
# COMMON OBJ FILES
$(COMMON_OBJ) : $(SRC)tcp_common.c $(COMMON_H)
	$(CC) $(INC_FLAGS) $(C_FLAGS) -c $< -o $@
# BEEJ OBJ FILES
$(BEEJ_S_OBJ) : $(SRC)beej_s.c $(BEEJ_H)
	$(CC) $(C_FLAGS) -c $< -o $@
$(BEEJ_C_OBJ) : $(SRC)beej_c.c $(BEEJ_H)
	$(CC) $(C_FLAGS) -c $< -o $@
# TEST OBJ FILES
$(TEST_OBJ) : $(SRC)test.c $(COMMON_H) 
	$(CC) $(INC_FLAGS) $(C_FLAGS) -c $< -o $@
clean:
	rm -f $(ALL_OBJ) 
	rmdir $(OBJ)
	rm -f $(EXEC) test
