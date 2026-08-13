
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "libbres/point.h"
#include "libbres/scanline.h"
#include "libbres/bres.h"

#include "tri.h"

void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3,
    uint32_t color, span_func_t span_fn, void *arg)
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
		span_fn(arg, slist->list[i].x1, slist->list[i].x2, slist->list[i].y, color);
	}

	scanline_list_free(slist);
}

#define	WIDTH 1280
#define HEIGHT 1024
#define MAX_TRI_SIZE 64

void
benchmark_triangle(span_setup_func_t setup_fn, span_func_t fn, void *arg,
     int num_triangles)
{
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
		uint32_t color = rand() % 0xffffff;

		if (setup_fn != NULL)
			setup_fn(arg, color);

		draw_triangle(x1, y1, x2, y2, x3, y3, color, fn, arg);
	}
}
