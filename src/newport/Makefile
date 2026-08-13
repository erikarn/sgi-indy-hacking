CFLAGS=-O2 -g -ggdb -Wall -I../../lib
all: server triangle span-triangle line

OBJS=newport_regio.o newport_ops.o newport_hwops.o

server: $(OBJS) srv.o
	$(CC) -o server $(OBJS) srv.o

triangle: $(OBJS) triangle.o tri.o
	$(CC) -o triangle $(OBJS) triangle.o tri.o ../../lib/libbres/libbres.a

span-triangle: $(OBJS) span-triangle.o
	$(CC) -o span-triangle $(OBJS) span-triangle.o tri.o ../../lib/libbres/libbres.a

line: $(OBJS) line.o
	$(CC) -o line $(OBJS) line.o

clean:
	rm -f server triangle line span-triangle
	rm -f *.o
