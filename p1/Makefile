
CC=gcc
CFLAGS=-Wall -pthread -g

PROGS= producer

all: $(PROGS)

% : %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS) *.o *~

