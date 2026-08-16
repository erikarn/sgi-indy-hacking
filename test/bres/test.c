#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

#include "libpoint/point.h"
#include "libscanline/scanline.h"
#include "libbres/bres.h"

void
print_scanlines(struct scanline_list *slist)
{
	for (int i = 0; i < slist->count; i++) {
		printf("scanline %d: (%d, %d), y=%d\n",
		    i, slist->list[i].x1, slist->list[i].x2, slist->list[i].y);
	}
}

void
test_flat_triangle(int x1, int y1, int x2l, int x2r, int y2)
{
	struct point2d p[3];
	struct scanline_list *slist;

	p[0].x = x1; p[0].y = y1;
	p[1].x = x2l; p[1].y = y2;
	p[2].x = x2r; p[2].y = y2;

	printf("Testing flat triangle: (%d,%d) -> (%d,%d), (%d,%d)\n",
	    x1, y1, x2l, y2, x2r, y2);

	bres_triangle(p, &slist);
	print_scanlines(slist);
	scanline_list_free(slist);
	printf("\n");
}

void
test_arbitrary_triangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
	struct point2d p[3];
	struct scanline_list *slist;

	p[0].x = x1; p[0].y = y1;
	p[1].x = x2; p[1].y = y2;
	p[2].x = x3; p[2].y = y3;

	printf("Testing arbitrary triangle: (%d,%d), (%d,%d), (%d,%d)\n",
	    x1, y1, x2, y2, x3, y3);

	bres_triangle(p, &slist);
	print_scanlines(slist);
	scanline_list_free(slist);
	printf("\n");
}

int
main(int argc, const char *argv[])
{
	/* Flat bottom triangles (y2 > y1) */
	test_flat_triangle(40, 20, 5, 60, 30);
	test_flat_triangle(40, 20, 5, 10, 30);
	test_flat_triangle(40, 20, 50, 60, 30);

	/* Flat top triangles (y1 > y2) */
	test_flat_triangle(40, 30, 5, 60, 20);
	test_flat_triangle(40, 30, 5, 10, 20);
	test_flat_triangle(40, 30, 50, 60, 20);

	/* Arbitrary triangles */
	test_arbitrary_triangle(10, 10, 50, 60, 90, 20);
	test_arbitrary_triangle(100, 100, 150, 50, 200, 120);
	test_arbitrary_triangle(0, 0, 100, 200, 50, 150);

	return 0;
}
