#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

#include "libbres/scanline.h"
#include "bres_sub.h"

/*
 * Sub-pixel precision Bresenham triangle rasterizer.
 * Uses fixed-point arithmetic with SUBPIXEL_BITS fractional bits
 * for accurate edge walking, even for shallow slopes.
 */

static void
bres_sub_triangle_flat(struct scanline_list *slist, int x1, int y1, int x2l,
    int x2r, int y2)
{
	const int xl_sign = (x2l < x1) ? -1 : 1;
	const int xr_sign = (x2r < x1) ? -1 : 1;
	
	/* Use fixed-point for X coordinates */
	int64_t X_left_fp = (int64_t)x1 * SUBPIXEL_SCALE;
	int64_t X_right_fp = (int64_t)x1 * SUBPIXEL_SCALE;
	
	int yi;
	const int y_sign = (y2 < y1) ? -1 : 1;
	int dy = abs(y2 - y1);
	int dx_left = abs(x2l - x1);
	int dx_right = abs(x2r - x1);
	
	/* Fixed-point increments per row */
	int64_t inc_left_fp = ((int64_t)(x2l - x1) * SUBPIXEL_SCALE) / dy;
	int64_t inc_right_fp = ((int64_t)(x2r - x1) * SUBPIXEL_SCALE) / dy;

#ifdef DEBUG_TRIANGLE
	printf("%s:  (%d,%d) -> (%d,%d), (%d,%d) \n",
	    __func__, x1, y1, x2l, y2, x2r, y2);
	printf("%s: dy=%d, inc_left=%.4f, inc_right=%.4f\n", 
	       __func__, dy, 
	       (double)inc_left_fp / SUBPIXEL_SCALE,
	       (double)inc_right_fp / SUBPIXEL_SCALE);
#endif

	/* Handle degenerate case */
	if (y1 == y2) {
		int xmin = x1, xmax = x1;
		if (x2l < xmin) xmin = x2l;
		if (x2l > xmax) xmax = x2l;
		if (x2r < xmin) xmin = x2r;
		if (x2r > xmax) xmax = x2r;
		scanline_list_push(slist, xmin, xmax, y1);
		return;
	}

	for (yi = y1; yi != y2 + y_sign; yi += y_sign) {
		/* Extract integer parts for scanline */
		int X_left = (int)(X_left_fp >> SUBPIXEL_BITS);
		int X_right = (int)(X_right_fp >> SUBPIXEL_BITS);
		
		/* Round to nearest pixel */
		if (X_left_fp & (1 << (SUBPIXEL_BITS - 1)))
			X_left++;
		if (X_right_fp & (1 << (SUBPIXEL_BITS - 1)))
			X_right++;

#ifdef DEBUG_TRIANGLE
		printf("%s: yi=%d, X_left_fp=%lld (%d), X_right_fp=%lld (%d)\n", 
		       __func__, yi, 
		       (long long)X_left_fp, X_left,
		       (long long)X_right_fp, X_right);
#endif

		/* Ensure x1 <= x2 for valid span */
		if (X_left > X_right) {
			int tmp = X_left;
			X_left = X_right;
			X_right = tmp;
		}
		
		scanline_list_push(slist, X_left, X_right, yi);
		
		/* Advance using fixed-point increments */
		X_left_fp += inc_left_fp;
		X_right_fp += inc_right_fp;
	}
#ifdef DEBUG_TRIANGLE
	printf("%s: done\n", __func__);
#endif
}

void
bres_sub_triangle(struct point2d *plist, struct scanline_list **slist)
{
	struct point2d mp;
	int a = 0, b = 1, c = 2, t;
	
	/* Sort vertices by Y coordinate (bubble sort) */
	if (plist[a].y > plist[b].y) {
		t = a; a = b; b = t;
	}
	if (plist[b].y > plist[c].y) {
		t = b; b = c; c = t;
	}
	if (plist[a].y > plist[b].y) {
		t = a; a = b; b = t;
	}
	
	/* Allocate scanline list */
	*slist = scanline_list_alloc(plist[c].y - plist[a].y + 1);
	
	if (plist[b].y == plist[c].y) {
		/* Flat bottom triangle */
		bres_sub_triangle_flat(*slist, plist[a].x, plist[a].y,
		    plist[b].x, plist[c].x, plist[c].y);
	} else if (plist[a].y == plist[b].y) {
		/* Flat top triangle */
		bres_sub_triangle_flat(*slist, plist[c].x, plist[c].y,
		    plist[a].x, plist[b].x, plist[b].y);
	} else {
		/* Arbitrary triangle - split at middle vertex */
		/* Use fixed-point for midpoint calculation */
		int64_t dy = (int64_t)(plist[c].y - plist[a].y);
		int64_t dx = (int64_t)(plist[c].x - plist[a].x);
		int64_t by = (int64_t)(plist[b].y - plist[a].y);
		
		mp.y = plist[b].y;
		mp.x = plist[a].x + (int)((by * dx) / dy);
		
		/* Flat bottom: apex at a, base at b and mp */
		bres_sub_triangle_flat(*slist, plist[a].x, plist[a].y,
		    plist[b].x, mp.x, mp.y);
		
		/* Flat top: apex at c, base at b and mp (exclude split line) */
		bres_sub_triangle_flat(*slist, plist[c].x, plist[c].y,
		    plist[b].x, mp.x, plist[b].y + 1);
	}
}

void
bres_sub_triangle_xy(int x1, int y1, int x2, int y2, int x3, int y3,
    struct scanline_list **slist)
{
	struct point2d p[3];
	
	p[0].x = x1; p[0].y = y1;
	p[1].x = x2; p[1].y = y2;
	p[2].x = x3; p[2].y = y3;
	
	bres_sub_triangle(p, slist);
}
