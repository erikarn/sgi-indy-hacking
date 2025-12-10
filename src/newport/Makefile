CFLAGS=-O2 -g -ggdb -Wall
all: server

OBJS=srv.o newport_regio.o newport_ops.o

server: $(OBJS)
	$(CC) -o server $(OBJS)

clean:
	rm -f server
	rm -f *.o
