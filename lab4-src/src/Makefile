# Makefile for lab 4

CC=g++

CFLAGS= -g

all: git-commit lab4

TestRWLock.o: TestRWLock.cc RWLock.h script.h
	$(CC) -c $(CFLAGS) TestRWLock.cc

RWLock.o: RWLock.cc RWLock.h
	$(CC) -c $(CFLAGS) RWLock.cc

lab4: TestRWLock.o RWLock.o
	$(CC) $(CFLAGS) -o lab4 TestRWLock.o RWLock.o -lpthread

git-commit:
	git add *.cc *.c *.h Makefile >> .local.git.out  || echo
	git commit -a -m 'Commit' >> .local.git.out || echo
	git push


clean:
	rm -f lab4 *.o


