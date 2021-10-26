CC=gcc
CFLAGS=-std=c99 -g

#
# TODO
#

all: fibonacci

fibonacci: fib.c main.c
	$(CC) $(CFLAGS) -o $@ main.c fib.c

clean:
	rm -f fibonacci

