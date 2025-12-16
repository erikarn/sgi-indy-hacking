
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>

#include "point.h"
#include "scanline.h"
#include "bres.h"

#define WIDTH 640
#define HEIGHT 480

SDL_Window* window;
SDL_Surface* surface;

uint32_t* pixels;
bool keys[512];

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t)SDL_MapRGB(surface->format, r, g, b);
}

void pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    pixels[y * WIDTH + x] = color;
}

void span(int x1, int x2, int y1, uint32_t lc, uint32_t rc, uint32_t mc)
{
	int i;
	for (i = x1; i <= x2; i++) {
		if (i == x1)
			pixel(i, y1, lc);
		else if (i == x2)
			pixel(i, y1, rc);
		else
			pixel(i, y1, mc);
	}
}

void
do_triangle(int x1, int y1, int x2, int y2, int x3, int y3,
    uint32_t lc, uint32_t rc, uint32_t mc)
{
	struct scanline_list *sl = NULL;
	int i;

	bres_triangle_xy(x1, y1, x2, y2, x3, y3, &sl);

	if (sl == NULL)
		return;

	for (i = 0; i < sl->cur; i++) {
		span(sl->list[i].x1, sl->list[i].x2, sl->list[i].y, lc, rc, mc);
	}

	scanline_list_print(sl, "triangle: ");

	scanline_list_free(sl);

}

int
main(int argc, const char *argv[])
{
	bool quit = false;
	SDL_Event e;

	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED,
	    SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	surface = SDL_GetWindowSurface(window);
	pixels = (uint32_t*)surface->pixels;


/* clear screen */
SDL_LockSurface(surface);
memset(pixels, 0, sizeof(uint32_t) * WIDTH * HEIGHT);

SDL_UnlockSurface(surface);
SDL_UpdateWindowSurface(window);

SDL_LockSurface(surface);

//do_triangle(50, 50, 100, 100, 30, 120, 0x0000ff, 0x00ff00, 0xff0000);
do_triangle(50, 50, 30, 70, 40, 80, 0x0000ff, 0x00ff00, 0xff0000);
// test_triangle_xy(50, 50, 30, 70, 40, 80);


SDL_UnlockSurface(surface);
SDL_UpdateWindowSurface(window);

/* XXX why consume all the cpu? sigh */
while (!quit) {
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) quit = true;
        else if (e.type == SDL_KEYDOWN) keys[e.key.keysym.scancode] = true;
        else if (e.type == SDL_KEYUP) keys[e.key.keysym.scancode] = false;
    }
}

SDL_DestroyWindow(window);
SDL_Quit();
}
