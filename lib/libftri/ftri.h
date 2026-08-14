#ifndef	__FTRI_H__
#define	__FTRI_H__

#include "libbres/point.h"

struct scanline_list;

extern	void ftri_triangle(struct point2d *plist, struct scanline_list **slist);
extern	void ftri_triangle_xy(int x1, int y1, int x2, int y2, int x3, int y3,
	    struct scanline_list **slist);

#endif	/* __FTRI_H__ */
