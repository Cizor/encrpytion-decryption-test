CC=gcc
CC_OPTIONS= -g -lpthread -lssl -lcrypt -lcrypto -o 
CLIENT_SRC_FILE= Client.c
SERVER_SRC_FILE= Server.c
CLIENT_EXE= client
SERVER_EXE= server
RM= rm -f

default:all

all:server_gen client_gen

server_gen:
	$(CC) $(SERVER_SRC_FILE) $(CC_OPTIONS) $(SERVER_EXE)

client_gen:
	$(CC) $(CLIENT_SRC_FILE) $(CC_OPTIONS) $(CLIENT_EXE)

clean:
	$(RM) $(CLIENT_EXE) $(SERVER_EXE)
