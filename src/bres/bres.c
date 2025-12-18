#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

#include "point.h"
#include "scanline.h"
#include "bres.h"

//#define DEBUG_TRIANGLE
#define DRAW_TRIANGLE
//#define PRINT_TRIANGLE_SETUP

/*
 * Experiments rendering a flat top or bottom triangle
 * from (x1,y1) -> (x2l, y2) / (x2r, y2)
 *
 * This attempts to handle all the possible configurations:
 *
 * flat bottom (y1 < y2)
 * + xl < x < xr
 * + xl < xr < x
 * + x < xl < xr
 *
 * flat top (y1 > y2)
 * + xl < x < xr
 * + xl < xr < x
 * + x < xl < xr
 *
 * I haven't yet tested the degenerate cases (y1 == y2, x1 == x,
 * x2 == x, x1 == x == x2).
 *
 * Inspired by https://mcejp.github.io/2020/11/06/bresenham.html
 *
 * TODO: do i have all the error terms correct?
 */
void
bres_triangle_flat(struct scanline_list *slist, int x1, int y1, int x2l,
     int x2r, int y2)
{
	const int xl_sign = (x2l < x1) ? -1 : 1;
	const int xr_sign = (x2r < x1) ? -1 : 1;
	int X_left = x1;
	int X_right = x1;
	int yi;
	const int y_sign = (y2 < y1) ? -1 : 1;
	int yc = (y2 - y1) * y_sign;
	int E_left = -((y2 - y1) * y_sign) - (x2l - x1);
	int E_right = -((y2 - y1) * y_sign) - (x2r - x1);

#ifdef DEBUG_TRIANGLE
	printf("%s:  (%d,%d) -> (%d,%d), (%d,%d) \n",
	    __func__, x1, y1, x2l, y2, x2r, y2);
	printf("%s: xl_sign=%d, xr_sign=%d, y_sign=%d\n", __func__, xl_sign, xr_sign, y_sign);
#endif

	/*
	 * Handle y1 == y2; but ideally the caller would not bother and
	 * just render a single span.
	 */
	if (y1 == y2) {
		scanline_list_push(slist, X_left, X_right, y1);
		return;
	}

	/* TODO: is yc >= 0 correct? or yc > 0, and we need to add the final line/point? */
	for (yi = y1; yc >= 0; yc--, yi += y_sign) {
#ifdef DEBUG_TRIANGLE
		printf("%s:  e_left=%d, e_right=%d\n", __func__, E_left, E_right);
#endif
		/* left hand iterator */
		while (E_left >= 0) {
			X_left += xl_sign;
			E_left -= 2* ((y2 - y1) * y_sign);
		}

		/* right hand iterator */
		while (E_right >= 0) {
			X_right += xr_sign;
			E_right -= 2* ((y2 - y1) * y_sign);
		}
#ifdef DEBUG_TRIANGLE
#endif

		scanline_list_push(slist, X_left, X_right, yi);

		E_left -= 2 * -xl_sign * (x2l - x1);
		E_right -= 2 * -xr_sign * (x2r - x1);
	}
#ifdef DEBUG_TRIANGLE
	printf("%s: done\n", __func__);
#endif
}

/**
 * Given a triangle (x1,y1), (x2,y2), (x3,y3), generate the
 * scan list.
 */
void
bres_triangle(struct point2d *plist, struct scanline_list **slist)
{
	struct point2d mp;
	bool valid_mp;

	int a = 0, b = 1, c = 2, t;

	/* Assume that plist[0,1,2] contain the triangle points */

	/*
	 * Step one - sort them by y, since we need to find the
	 * midpoint that may become two triangles.
	 */
	if (plist[a].y > plist[b].y) {
		t = b; a = b; b = a;
	}
	if (plist[b].y > plist[c].y) {
		t = c; a = c; c = b;
	}

#ifdef PRINT_TRIANGLE_SETUP
	printf("%s: points: (%d, %d, %d) a=(%d,%d), b=(%d,%d), c=(%d,%d)\n",
	    __func__, a, b, c,
	    plist[a].x, plist[a].y,
	    plist[b].x, plist[b].y,
	    plist[c].x, plist[c].y);
#endif

	/* Figure out how big a scanlist to create */
	*slist = scanline_list_alloc(plist[c].y - plist[a].y);

	if (plist[b].y == plist[c].y) {
		/* Flat bottom triangle */
		/* plist[a] is the origin point, going to b, c */
#ifdef DRAW_TRIANGLE
		bres_triangle_flat(*slist, plist[a].x, plist[a].y,
		    plist[b].x, plist[c].x, plist[c].y);
#endif
	} else if (plist[a].y == plist[b].y) {
		/* Flat top triangle */
		/* plist[c] is the origin point, going to a, b */
#ifdef DRAW_TRIANGLE
		bres_triangle_flat(*slist, plist[c].x, plist[c].y,
		    plist[a].x, plist[b].x, plist[b].y);
#endif
	} else {
		int dx, dy, by, xinc;

		/* Split */
		/*
		 * Now for the midpoint (b), we need to create another point
		 * (mp) which bisects the long axis - which here should be
		 * (a -> c.) (mp) will have an x value between (a.x) and (c.x)
		 * at (b.y).
		 *
		 * The formula is mp.x = a.x + ((b.y-a.y)/(c.y-a.y))*(c.x-a.x)
		 * To avoid using floats we do a bit of fixed point math.
		 */
		mp.y = plist[b].y;
		dy = (plist[c].y - plist[a].y);
		dx = (plist[c].x - plist[a].x);
		by = (plist[b].y - plist[a].y) * 1024;
		xinc = (((by / dy) * dx)) / 1024;
		mp.x = plist[a].x + xinc;


#ifdef PRINT_TRIANGLE_SETUP
	printf("%s: [a].x=%d, xinc=%d\n", __func__, plist[a].x, xinc);
	printf("%s: mp.x=%d, mp.y=%d\n", __func__, mp.x, mp.y);
	printf("%s: dy=%d, dx=%d, by=%d\n", __func__, dy, dx, by);
#endif

#ifdef DRAW_TRIANGLE
		/* Flat bottom - a, b, mp; mp.y == b.y */
		bres_triangle_flat(*slist, plist[a].x, plist[a].y,
		    plist[b].x, mp.x, mp.y);
#endif

#ifdef DRAW_TRIANGLE
		/* Flat top - b, mp, c, mp.y == c.y */
		bres_triangle_flat(*slist, plist[c].x, plist[c].y,
		    plist[b].x, mp.x, plist[b].y);
#endif
	}
}

void
bres_triangle_xy(int x1, int y1, int x2, int y2, int x3, int y3,
    struct scanline_list **slist)
{
	struct point2d p[3];

	p[0].x = x1; p[0].y = y1;
	p[1].x = x2; p[1].y = y2;
	p[2].x = x3; p[2].y = y3;

	bres_triangle(p, slist);
}
