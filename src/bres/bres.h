#ifndef	__BRES_H__
#define	__BRES_H__

extern	void bres_triangle_flat(struct scanline_list *slist, int x1, int y1,
	    int x2l, int x2r, int y2);
extern	void bres_triangle_xy(int x1, int y1, int x2, int y2, int x3, int y3,
	    struct scanline_list **slist);

#endif	/* __BRES_H__ */
