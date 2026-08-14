include ../../Makefile.inc

SRCS = ftri.c
OBJS = ftri.o

LIB_STATIC = libftri.a
LIB_SHARED = libftri.so

CFLAGS += -I../../lib

all: $(LIB_STATIC) $(LIB_SHARED)

$(LIB_STATIC): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(LIB_SHARED): $(OBJS)
	$(CC) -shared -o $@ $^

ftri.o: ftri.c ftri.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(LIB_STATIC) $(LIB_SHARED)

.PHONY: all clean
