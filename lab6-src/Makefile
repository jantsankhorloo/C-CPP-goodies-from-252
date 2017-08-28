# Makefile for lab 6

CC=gcc

CFLAGS= -lkvm

FLAG= -lutil

all: arena arenas arenar fork spongebob patrick

arena: 
	$(CC) $(CFLAGS) src/arena.c -o arena

arenas: 
	$(CC) $(CFLAGS) src/arena_s.c -o arena_s

arenar: 
	$(CC) $(CFLAGS) src/arena_r.c -o arena_r

fork: 
	$(CC) $(CFLAGS) src/fork.c -o fork

spongebob:
	$(CC) $(CFLAGS) src/spongebob.c -o spongebob

patrick:
	$(CC) $(CFLAGS) $(FLAG) src/patrick.c -o patrick

clean:
	rm -f arena arena_s arena_r fork arena spongebob patrick