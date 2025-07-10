# Compiler and flags
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread


all: proxy


proxy: proxy_parse.o proxy_server_with_cache.o
	$(CC) $(CFLAGS) -o proxy proxy_parse.o proxy_server_with_cache.o $(LDFLAGS)


proxy_parse.o: proxy_parse.c proxy_parse.h
	$(CC) $(CFLAGS) -c proxy_parse.c -o proxy_parse.o


proxy_server_with_cache.o: proxy_server_with_cache.c proxy_parse.h
	$(CC) $(CFLAGS) -c proxy_server_with_cache.c -o proxy_server_with_cache.o


clean:
	rm -f proxy *.o


tar:
	tar -cvzf ass1.tgz proxy_server_with_cache.c README Makefile proxy_parse.c proxy_parse.h
