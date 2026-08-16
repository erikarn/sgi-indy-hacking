
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "libpoint/point.h"
#include "libscanline/scanline.h"
#include "libbres/bres.h"

#define WIDTH 800
#define HEIGHT 600
#define NUM_TRIANGLES 1000
#define MAX_TRI_SIZE 64

SDL_Window* window;
SDL_Surface* surface;
uint32_t* pixels;

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t)SDL_MapRGB(surface->format, r, g, b);
}

void pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    pixels[y * WIDTH + x] = color;
}

void span(int x1, int x2, int y, uint32_t color)
{
	for (int i = x1; i <= x2; i++) {
		pixel(i, y, color);
	}
}

void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color)
{
	struct point2d p[3];
	struct scanline_list *slist;

	p[0].x = x1; p[0].y = y1;
	p[1].x = x2; p[1].y = y2;
	p[2].x = x3; p[2].y = y3;

	bres_triangle(p, &slist);

	if (slist == NULL)
		return;

	for (int i = 0; i < slist->count; i++) {
		span(slist->list[i].x1, slist->list[i].x2, slist->list[i].y, color);
	}

	scanline_list_free(slist);
}

int main(int argc, const char *argv[])
{
	bool quit = false;
	SDL_Event e;

	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Random Triangles", SDL_WINDOWPOS_UNDEFINED,
	    SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	surface = SDL_GetWindowSurface(window);
	pixels = (uint32_t*)surface->pixels;

	srand(time(NULL));

	/* Clear screen to black */
	SDL_LockSurface(surface);
	memset(pixels, 0, sizeof(uint32_t) * WIDTH * HEIGHT);

	/* Parse command line args */
	int num_triangles = NUM_TRIANGLES;
	if (argc > 1) {
		num_triangles = atoi(argv[1]);
		if (num_triangles <= 0)
			num_triangles = NUM_TRIANGLES;
	}

	/* Draw random triangles with periodic screen updates */
	int update_interval = num_triangles / 100;
	if (update_interval < 100)
		update_interval = 100;

	for (int i = 0; i < num_triangles; i++) {
		/* Random position within canvas bounds */
		int base_x = rand() % (WIDTH - MAX_TRI_SIZE);
		int base_y = rand() % (HEIGHT - MAX_TRI_SIZE);

		/* Random triangle vertices within 64x64 box */
		int x1 = base_x + (rand() % MAX_TRI_SIZE);
		int y1 = base_y + (rand() % MAX_TRI_SIZE);
		int x2 = base_x + (rand() % MAX_TRI_SIZE);
		int y2 = base_y + (rand() % MAX_TRI_SIZE);
		int x3 = base_x + (rand() % MAX_TRI_SIZE);
		int y3 = base_y + (rand() % MAX_TRI_SIZE);

		/* Random color */
		uint8_t r = rand() % 256;
		uint8_t g = rand() % 256;
		uint8_t b = rand() % 256;
		uint32_t color = rgb(r, g, b);

		draw_triangle(x1, y1, x2, y2, x3, y3, color);

		/* Update screen periodically */
		if ((i + 1) % update_interval == 0) {
			SDL_UnlockSurface(surface);
			SDL_UpdateWindowSurface(window);
			SDL_LockSurface(surface);
		}
	}

	SDL_UnlockSurface(surface);
	SDL_UpdateWindowSurface(window);

	while (!quit) {
		SDL_WaitEvent(&e);
		if (e.type == SDL_QUIT) quit = true;
	}

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
