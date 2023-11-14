CC = gcc
SRC = src
INC = include
BIN = bin
OBJ = obj

CLIENT_SRCS = $(SRC)/client.c
SERVER_SRCS = $(SRC)/server.c
COMMON_SRCS = $(SRC)/common.c

CLIENT_OBJS = $(OBJ)/client.o
SERVER_OBJS = $(OBJ)/server.o
COMMON_OBJS = $(OBJ)/common.o

CLIENT_EXE = $(BIN)/client
SERVER_EXE = $(BIN)/server

HDRS = $(INC)/common.h

all: $(CLIENT_EXE) $(SERVER_EXE)

$(CLIENT_EXE): $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CC) -g -o $(CLIENT_EXE) $(CLIENT_OBJS) $(COMMON_OBJS)

$(SERVER_EXE): $(SERVER_OBJS) $(COMMON_OBJS)
	$(CC) -g -o $(SERVER_EXE) $(SERVER_OBJS) $(COMMON_OBJS)

$(OBJ)/client.o: $(HDRS) $(CLIENT_SRCS) 
	$(CC) -g -o $(OBJ)/client.o -c $(CLIENT_SRCS) -I$(INC)

$(OBJ)/server.o: $(HDRS) $(SERVER_SRCS)
	$(CC) -g -o $(OBJ)/server.o -c $(SERVER_SRCS) -I$(INC)

$(OBJ)/common.o: $(COMMON_SRCS)
	$(CC) -g -o $(OBJ)/common.o -c $(COMMON_SRCS) -I$(INC)

clean:
	rm $(CLIENT_EXE) $(SERVER_EXE) $(CLIENT_OBJS) $(SERVER_OBJS) $(COMMON_OBJS)
