
CC=gcc
CFLAGS=-Wall -pthread -g -lrt

all:barber

barber:barber.o queue.o
	$(CC) $(CFLAGS) -o barber barber.o queue.o

barber.o:barber.c queue.c queue.h
	$(CC) $(CFLAGS) -c barber.c

queue.o:queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

clean:
	rm -f *.o *~
