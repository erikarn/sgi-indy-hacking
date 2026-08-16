include ../../Makefile.inc

TARGETS = sdl triangles

LIBDIR = ../../lib/libbres
FTRIDIR = ../../lib/libftri
LDFLAGS += -L/usr/local/lib -L$(FTRIDIR)
LDLIBS = $(LIBDIR)/libbres.a $(FTRIDIR)/libftri.a -lSDL2

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

TARGETS = sdl triangles triangle_viewer

triangle_viewer: triangle_viewer.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

triangle_viewer.o: triangle_viewer.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
