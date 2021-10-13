CFLAGS=-Wall -O2

all: 1.unixio 2.stdio 3.stdio.analysis

%: %.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f 1.unixio 2.stdio 3.stdio.analysis

