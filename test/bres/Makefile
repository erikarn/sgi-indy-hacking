include ../../Makefile.inc

TARGETS = test test_sorting

LIBDIR = ../../lib/libbres
LDFLAGS += -L$(LIBDIR)
LDLIBS = -lbres

CFLAGS += -I../../lib

all: $(TARGETS)

test: test.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBDIR)/libbres.a

test_sorting: test_sorting.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBDIR)/libbres.a

test.o: test.c
	$(CC) $(CFLAGS) -c -o $@ $<

test_sorting.o: test_sorting.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
