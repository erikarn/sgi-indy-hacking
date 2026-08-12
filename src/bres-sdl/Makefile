include ../../Makefile.inc

TARGET = sdl

SRCS = sdl.c
OBJS = sdl.o

LIBDIR = ../../lib/libbres
LDFLAGS += -L/usr/local/lib
LDLIBS = $(LIBDIR)/libbres.a -lSDL2

CFLAGS += -I../../lib -I/usr/local/include

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

sdl.o: sdl.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
