#ifndef	__SCANLINE_H__
#define	__SCANLINE_H__

struct scanline_2d {
	int x1, x2, y;
};

struct scanline_list {
	int count;
	int cur;
	struct scanline_2d *list;
};

extern	struct scanline_list *scanline_list_alloc(int count);
extern	void scanline_list_free(struct scanline_list *);
extern	bool scanline_list_push(struct scanline_list *, int x1, int x2, int y);
extern	void scanline_list_print(const struct scanline_list *l,
	    const char *pfx);

#endif	/* __SCANLINE_H__ */
