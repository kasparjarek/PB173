CC=gcc
CFLAGS=-std=c99 -Wall -pedantic

all: main lib clean

main: main.c
	$(CC) $(CFLAGS) main.c -o main

lib: foo.c bar.c
	$(CC) $(CFLAGS) -fPIC -c foo.c
	$(CC) $(CFLAGS) -fPIC -c bar.c
	$(CC) -shared -Wl foo.o -o libfoo.so.1.0.0
	$(CC) -shared -Wl bar.o -o libbar.so.1.0.0

clean: lib
	rm *.o

.PHONY: lib clean all