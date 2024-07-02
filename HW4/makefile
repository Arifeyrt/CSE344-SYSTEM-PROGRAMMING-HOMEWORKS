CC = gcc
CFLAGS = -pthread -Wall

all: MWCp

MWCp: 210104004294_main.o
	$(CC) $(CFLAGS) -o MWCp 210104004294_main.o

210104004294_main.o: 210104004294_main.c
	$(CC) $(CFLAGS) -c 210104004294_main.c

clean:
	rm -f *.o MWCp