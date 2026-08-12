CFLAGS=-O2 -g -ggdb -Wall
all: server triangle line

OBJS=newport_regio.o newport_ops.o newport_hwops.o

server: $(OBJS) srv.o
	$(CC) -o server $(OBJS) srv.o

triangle: $(OBJS) triangle.o
	$(CC) -o triangle $(OBJS) triangle.o

line: $(OBJS) line.o
	$(CC) -o line $(OBJS) line.o

clean:
	rm -f server triangle line
	rm -f *.o
