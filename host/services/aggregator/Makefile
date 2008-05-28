CC:=$(CROSS)gcc
CFLAGS:=-fpack-struct=1 


all: aggregator

aggregator: main.o
	$(CC) $(LDFLAGS) $(CFLAGS) main.o -o aggregator

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f aggregator *~ *.o
