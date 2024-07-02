CC=gcc
CFLAGS=-g -Wall -Wextra
LDFLAGS=

all: neHosServer neHosClient

neHosClient: client.c Fifostruct.h -lpthread
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

neHosServer: server.c serverfunction.c Fifostruct.h -lpthread
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f neHosServer neHosClient *.txt

.PHONY: all
