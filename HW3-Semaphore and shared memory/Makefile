CC=gcc
CFLAGS=-g -Wall -Wextra
LDFLAGS=

all: main

main: main.c -lpthread
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f main *.txt

.PHONY: all
