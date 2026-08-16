#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

#include "libscanline/scanline.h"
#include "ftri.h"

/*
 * Linear interpolation triangle rasterizer.
 * Uses floating-point arithmetic for accurate edge walking.
 */

static void
ftri_triangle_flat(struct scanline_list *slist, int x1, int y1, int x2l,
    int x2r, int y2)
{
	double slope_left, slope_right;
	int yi;
	
	/* Handle degenerate case */
	if (y1 == y2) {
		int xmin = x1 < x2l ? x1 : x2l;
		int xmax = x1 > x2r ? x1 : x2r;
		if (x2l < xmin) xmin = x2l;
		if (x2r > xmax) xmax = x2r;
		scanline_list_push(slist, xmin, xmax, y1);
		return;
	}
	
	/* Calculate edge slopes */
	slope_left = (double)(x2l - x1) / (y2 - y1);
	slope_right = (double)(x2r - x1) / (y2 - y1);
	
	/* Determine direction */
	int y_step = (y2 > y1) ? 1 : -1;
	int num_rows = abs(y2 - y1);
	
	for (int i = 0; i <= num_rows; i++) {
		yi = y1 + i * y_step;
		
		/* Interpolate X coordinates */
		int xl = (int)(x1 + slope_left * i * y_step);
		int xr = (int)(x1 + slope_right * i * y_step);
		
		/* Ensure x1 <= x2 */
		if (xl > xr) {
			int tmp = xl;
			xl = xr;
			xr = tmp;
		}
		
		scanline_list_push(slist, xl, xr, yi);
	}
}

void
ftri_triangle(struct point2d *plist, struct scanline_list **slist)
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
		ftri_triangle_flat(*slist, plist[a].x, plist[a].y,
		    plist[b].x, plist[c].x, plist[c].y);
	} else if (plist[a].y == plist[b].y) {
		/* Flat top triangle */
		ftri_triangle_flat(*slist, plist[c].x, plist[c].y,
		    plist[a].x, plist[b].x, plist[b].y);
	} else {
		/* Arbitrary triangle - split at middle vertex */
		double dy = (double)(plist[c].y - plist[a].y);
		double dx = (double)(plist[c].x - plist[a].x);
		double ratio = (double)(plist[b].y - plist[a].y) / dy;
		
		mp.y = plist[b].y;
		mp.x = (int)(plist[a].x + dx * ratio);
		
		/* Flat bottom: apex at a, base at b and mp */
		ftri_triangle_flat(*slist, plist[a].x, plist[a].y,
		    plist[b].x, mp.x, mp.y);
		
		/* Flat top: apex at c, base at b and mp (exclude split line) */
		ftri_triangle_flat(*slist, plist[c].x, plist[c].y,
		    plist[b].x, mp.x, plist[b].y + 1);
	}
}

void
ftri_triangle_xy(int x1, int y1, int x2, int y2, int x3, int y3,
    struct scanline_list **slist)
{
	struct point2d p[3];
	
	p[0].x = x1; p[0].y = y1;
	p[1].x = x2; p[1].y = y2;
	p[2].x = x3; p[2].y = y3;
	
	ftri_triangle(p, slist);
}
