include ../../Makefile.inc

SRCS = bres.c scanline.c
OBJS = bres.o scanline.o

LIB_STATIC = libbres.a
LIB_SHARED = libbres.so

all: $(LIB_STATIC) $(LIB_SHARED)

$(LIB_STATIC): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(LIB_SHARED): $(OBJS)
	$(CC) -shared -o $@ $^

bres.o: bres.c bres.h point.h scanline.h
	$(CC) $(CFLAGS) -c -o $@ $<

scanline.o: scanline.c scanline.h point.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(LIB_STATIC) $(LIB_SHARED)

.PHONY: all clean
