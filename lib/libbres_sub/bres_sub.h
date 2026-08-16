#ifndef	__BRES_SUB_H__
#define	__BRES_SUB_H__

/* Sub-pixel precision: 2^SUBPIXEL_BITS fractional bits */
#ifndef SUBPIXEL_BITS
#define SUBPIXEL_BITS 8
#endif

#define SUBPIXEL_SCALE (1 << SUBPIXEL_BITS)
#define SUBPIXEL_MASK (SUBPIXEL_SCALE - 1)

#include "libpoint/point.h"

struct scanline_list;

extern	void bres_sub_triangle(struct point2d *plist, struct scanline_list **slist);
extern	void bres_sub_triangle_xy(int x1, int y1, int x2, int y2, int x3, int y3,
	    struct scanline_list **slist);

#endif	/* __BRES_SUB_H__ */
