#ifndef	__TRI_H__
#define	__TRI_H__

typedef void (*span_func_t)(void *arg, int x1, int x2, int y, uint32_t color);

extern void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color,
    span_func_t span_fn, void *arg);
extern	void benchmark_triangle(span_func_t fn, void *arg, int num_triangles);

#endif
