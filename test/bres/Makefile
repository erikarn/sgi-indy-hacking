include ../../Makefile.inc

TARGETS = test test_sorting test_compare

LIBDIR = ../../lib
LDFLAGS += -L$(LIBDIR)
LDLIBS = -lbres

CFLAGS += -I../../lib

all: $(TARGETS)

TEST_LIBS= $(LIBDIR)/libbres/libbres.a \
	    $(LIBDIR)/libftri/libftri.a \
	    $(LIBDIR)/libscanline/libscanline.a \
	    $(LIBDIR)/libbres_sub/libbres_sub.a \

test_compare: test_compare.o
	$(CC) $(LDFLAGS) -o $@ $^ $(TEST_LIBS) -lm

test_compare.o: test_compare.c
	$(CC) $(CFLAGS) -I../../lib/libbres_sub -c -o $@ $<

test: test.o
	$(CC) $(LDFLAGS) -o $@ $^ $(TEST_LIBS)

test_sorting: test_sorting.o
	$(CC) $(LDFLAGS) -o $@ $^ $(TEST_LIBS)

test.o: test.c
	$(CC) $(CFLAGS) -c -o $@ $<

test_sorting.o: test_sorting.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
