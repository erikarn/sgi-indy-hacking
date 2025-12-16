
CFLAGS=-I/usr/local/include -O -g -ggdb
LDFLAGS=-L/usr/local/lib

all: test sdl

sdl: sdl.o
	$(CC) sdl.o -o sdl $(LDFLAGS) -lSDL2

clean:
	rm -f *.o test sdl


