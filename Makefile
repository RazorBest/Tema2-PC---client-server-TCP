# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile

CC = gcc
CFLAGS = -Wall -g

# Portul pe care asculta serverul (de completat)
PORT = 2048 

IP_SERVER = 127.0.0.1

SERVER_SOURCE_FILES = \
	Hashtable.c \

CLIENT_SOURCE_FILES = Hashtable.c
 
SINGLE_HEADERS = \
	utils.h \

all: server

SERVER_OBJ_FILES = $(SERVER_SOURCE_FILES:.c=.o)
CLIENT_OBJ_FILES = $(CLIENT_SOURCE_FILES:.c=.o)

# Compileaza server.c
server: $(SERVER_OBJ_FILES) server.c
	$(CC) $(CFLAGS) $^ -o $@

# Compileaza client.c
client: $(CLIENT_OBJ_FILES)
	$(CC) $(CFLAGS) $< -o $@


.PHONY: clean run_server run_client

%.o : %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_client:
	./client ${IP_SERVER} ${PORT}

clean:
	rm -f server client *.o
