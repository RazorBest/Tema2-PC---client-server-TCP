# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile

CC = g++
CFLAGS = -w -Wall

# Portul pe care asculta serverul (de completat)

SERVER_SOURCE_FILES = \
	tcp_app.cpp \
	subscriber.cpp \
	utils.cpp \
	BufferedTcpSocket.cpp \
	MultiSocket.cpp \

CLIENT_SOURCE_FILES = \
	tcp_app.cpp \
	subscriber.cpp \
	utils.cpp \
	BufferedTcpSocket.cpp \
	MultiSocket.cpp \
 
# SINGLE_HEADERS =

all: server subscriber

SERVER_OBJ_FILES = $(SERVER_SOURCE_FILES:.c=.o)
CLIENT_OBJ_FILES = $(CLIENT_SOURCE_FILES:.c=.o)

# Compileaza server.c
server: $(SERVER_OBJ_FILES) server.cpp
	$(CC) $(CFLAGS) $^ -o $@

# Compileaza client.c
subscriber: $(CLIENT_OBJ_FILES) client.cpp
	$(CC) $(CFLAGS) $^ -o $@


.PHONY: clean run_server run_client

%.o : %.cpp %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f server subscriber *.o
