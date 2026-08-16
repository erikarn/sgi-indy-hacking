
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libbres/point.h"
#include "libbres/scanline.h"
#include "libftri/ftri.h"

#define WIDTH 1280
#define HEIGHT 1024

SDL_Window* window;
SDL_Surface* surface;
SDL_Renderer* renderer;
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

void draw_triangle_from_coords(int x1, int y1, int x2, int y2, int x3, int y3, 
                                uint32_t color)
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

void draw_triangle_ftri_coords(int x1, int y1, int x2, int y2, int x3, int y3,
                                uint32_t color)
{
	struct point2d p[3];
	struct scanline_list *slist;

	p[0].x = x1; p[0].y = y1;
	p[1].x = x2; p[1].y = y2;
	p[2].x = x3; p[2].y = y3;

	ftri_triangle(p, &slist);

	if (slist == NULL)
		return;

	for (int i = 0; i < slist->count; i++) {
		span(slist->list[i].x1, slist->list[i].x2, slist->list[i].y, color);
	}

	scanline_list_free(slist);
}

void draw_triangle_sdl_filled(int x1, int y1, int x2, int y2, int x3, int y3,
                               uint32_t color)
{
	/* Simple scanline fill without Bresenham - for comparison */
	/* Sort vertices by Y */
	int tmp;
	if (y1 > y2) { tmp=y1; y1=y2; y2=tmp; tmp=x1; x1=x2; x2=tmp; }
	if (y2 > y3) { tmp=y2; y2=y3; y3=tmp; tmp=x2; x2=x3; x3=tmp; }
	if (y1 > y2) { tmp=y1; y1=y2; y2=tmp; tmp=x1; x1=x2; x2=tmp; }
	
	/* Now y1 <= y2 <= y3 */
	
	/* Calculate edge slopes as floating point */
	double slope_left, slope_right;
	int start_x, end_x;
	
	/* Upper part (y1 to y2) */
	if (y2 != y1) {
		slope_left = (double)(x2 - x1) / (y2 - y1);
	} else {
		slope_left = 0;
	}
	
	if (y3 != y1) {
		slope_right = (double)(x3 - x1) / (y3 - y1);
	} else {
		slope_right = 0;
	}
	
	for (int y = y1; y <= y2; y++) {
		start_x = x1 + (int)(slope_left * (y - y1));
		end_x = x1 + (int)(slope_right * (y - y1));
		
		if (start_x > end_x) {
			tmp = start_x;
			start_x = end_x;
			end_x = tmp;
		}
		
		for (int x = start_x; x <= end_x; x++) {
			pixel(x, y, color);
		}
	}
	
	/* Lower part (y2 to y3) */
	if (y3 != y2) {
		slope_left = (double)(x3 - x2) / (y3 - y2);
	} else {
		slope_left = 0;
	}
	
	for (int y = y2; y <= y3; y++) {
		start_x = x2 + (int)(slope_left * (y - y2));
		end_x = x1 + (int)(slope_right * (y - y1));
		
		if (start_x > end_x) {
			tmp = start_x;
			start_x = end_x;
			end_x = tmp;
		}
		
		for (int x = start_x; x <= end_x; x++) {
			pixel(x, y, color);
		}
	}
}

/* Parse a line like: triangle: (314,676), (344,618), (294,672) */
int parse_triangle_line(const char *line, int *x1, int *y1, int *x2, int *y2, 
                        int *x3, int *y3)
{
	const char *p = strstr(line, "triangle:");
	if (!p)
		return -1;
	
	p += 9; /* skip "triangle:" */
	
	if (sscanf(p, " (%d,%d), (%d,%d), (%d,%d)", x1, y1, x2, y2, x3, y3) == 6)
		return 0;
	
	return -1;
}

int main(int argc, const char *argv[])
{
	bool quit = false;
	SDL_Event e;
	FILE *fp;
	char line[512];
	int tri_count = 0;
	
	/* Default color cycle for triangles */
	uint32_t colors[] = {
		0x0000FF,      /* Red (BGR format) */
		0x00FF00,      /* Green */
		0xFF0000,      /* Blue */
		0x00FFFF,      /* Yellow */
		0xFF00FF,      /* Magenta */
		0xFFFF00,      /* Cyan */
		0x0080FF,      /* Orange */
		0x8000FF,      /* Purple */
	};
	int num_colors = sizeof(colors) / sizeof(colors[0]);

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <triangle_file.txt>\n", argv[0]);
		return 1;
	}

	fp = fopen(argv[1], "r");
	if (!fp) {
		perror("Failed to open file");
		return 1;
	}

	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Triangle Viewer", SDL_WINDOWPOS_UNDEFINED,
	    SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	surface = SDL_GetWindowSurface(window);
	pixels = (uint32_t*)surface->pixels;

	/* Convert colors to proper format */
	for (int i = 0; i < num_colors; i++) {
		uint8_t r = (colors[i] >> 16) & 0xFF;
		uint8_t g = (colors[i] >> 8) & 0xFF;
		uint8_t b = colors[i] & 0xFF;
		colors[i] = rgb(r, g, b);
	}

	/* Clear screen to black */
	SDL_LockSurface(surface);
	memset(pixels, 0, sizeof(uint32_t) * WIDTH * HEIGHT);

	/* Read and parse triangles */
	int use_ftri = (argc > 2 && strcmp(argv[2], "--reference") == 0);
	
	while (fgets(line, sizeof(line), fp)) {
		int x1, y1, x2, y2, x3, y3;
		
		if (parse_triangle_line(line, &x1, &y1, &x2, &y2, &x3, &y3) == 0) {
			uint32_t color = colors[tri_count % num_colors];
			
			if (use_ftri) {
				draw_triangle_ftri_coords(x1, y1, x2, y2, x3, y3, color);
			} else {
				draw_triangle_from_coords(x1, y1, x2, y2, x3, y3, color);
			}
			
			tri_count++;
			
			/* Update every 10 triangles */
			if (tri_count % 10 == 0) {
				SDL_UnlockSurface(surface);
				SDL_UpdateWindowSurface(window);
				SDL_LockSurface(surface);
			}
		}
	}

	fclose(fp);

	SDL_UnlockSurface(surface);
	SDL_UpdateWindowSurface(window);

	printf("Rendered %d triangles\n", tri_count);

	while (!quit) {
		SDL_WaitEvent(&e);
		if (e.type == SDL_QUIT) quit = true;
	}

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
