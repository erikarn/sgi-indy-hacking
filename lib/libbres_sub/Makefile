include ../../Makefile.inc

SRCS = bres_sub.c
OBJS = bres_sub.o

LIB_STATIC = libbres_sub.a
LIB_SHARED = libbres_sub.so

CFLAGS += -I../../lib

# Sub-pixel precision bits (can be overridden: make SUBPIXEL_BITS=16)
CFLAGS += -DSUBPIXEL_BITS=$(SUBPIXEL_BITS)
SUBPIXEL_BITS ?= 8

all: $(LIB_STATIC) $(LIB_SHARED)

$(LIB_STATIC): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(LIB_SHARED): $(OBJS)
	$(CC) -shared -o $@ $^

bres_sub.o: bres_sub.c bres_sub.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(LIB_STATIC) $(LIB_SHARED)

.PHONY: all clean
