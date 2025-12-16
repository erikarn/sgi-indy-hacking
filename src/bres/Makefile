
CFLAGS=-I/usr/local/include -O -g -ggdb
LDFLAGS=-L/usr/local/lib

all: test sdl

test: test.o

sdl: sdl.o bres.o scanline.o
	$(CC) sdl.o bres.o scanline.o -o sdl $(LDFLAGS) -lSDL2

clean:
	rm -f *.o test sdl


