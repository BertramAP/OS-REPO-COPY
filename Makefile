CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread

all: server

server: server.c
	$(CC) $(CFLAGS) -o server server.c

clean:
	rm -f server *.o