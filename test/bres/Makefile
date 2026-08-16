include ../../Makefile.inc

TARGETS = test test_sorting test_compare

test_compare: test_compare.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBDIR)/libbres.a ../../lib/libftri/libftri.a ../../lib/libbres_sub/libbres_sub.a -lm

test_compare.o: test_compare.c
	$(CC) $(CFLAGS) -I../../lib/libbres_sub -c -o $@ $<

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
