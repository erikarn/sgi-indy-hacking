include ../../Makefile.inc

TARGETS = sdl triangles

LIBDIR = ../../lib/libbres
LDFLAGS += -L/usr/local/lib
LDLIBS = $(LIBDIR)/libbres.a -lSDL2

CFLAGS += -I../../lib -I/usr/local/include

all: $(TARGETS)

sdl: sdl.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

triangles: triangles.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

sdl.o: sdl.c
	$(CC) $(CFLAGS) -c -o $@ $<

triangles.o: triangles.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
