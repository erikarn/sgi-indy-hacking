include ../../Makefile.inc

TARGET = test

SRCS = test.c
OBJS = test.o

LIBDIR = ../../lib/libbres
LDFLAGS += -L$(LIBDIR)
LDLIBS = -lbres

CFLAGS += -I../../lib

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test.o: test.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
