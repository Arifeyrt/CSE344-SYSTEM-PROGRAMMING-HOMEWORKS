CC = gcc
CFLAGS = -pthread -Wall

all: MWCp

MWCp: 210104004294_main.o
	$(CC) $(CFLAGS) -o MWCp 210104004294_main.o

210104004294_main.o: 210104004294_main.c
	$(CC) $(CFLAGS) -c 210104004294_main.c

clean:
	rm -f *.o MWCp

# test1: all
# 	@echo "Running Test 1: buffer size=10, number of workers=10, using valgrind for memory leak checking"
# 	valgrind ./MWCp 10 10 ../testdir/src/libvterm ../tocopy

# test2: all
# 	@echo "Running Test 2: buffer size=10, number of workers=4"
# 	./MWCp 10 4 ../testdir/src/libvterm/src ../toCopy

# test3: all
# 	@echo "Running Test 3: buffer size=10, number of workers=10"
# 	./MWCp 10 10 ../testdir ../toCopy
