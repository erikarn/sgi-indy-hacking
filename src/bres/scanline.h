#ifndef	__SCANLINE_H__
#define	__SCANLINE_H__

struct scanline_2d {
	int x1, x2, y;
};

struct scanline_list {
	int count;
	struct scanline_2d *list;
};

extern	struct scanline_list *scanline_list_alloc(int count);
extern	void scanline_list_free(struct scanline_list *);

#endif	/* __SCANLINE_H__ */
