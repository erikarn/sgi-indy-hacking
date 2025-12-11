#ifndef	__NEWPORT_OPS_H__
#define	__NEWPORT_OPS_H__

extern	uint32_t newport_calc_drawmode1(struct gfx_ctx *ctx);
extern	uint32_t newport_calc_wrmode(struct gfx_ctx *ctx,
	    uint32_t planemask);
extern	uint32_t newport_calc_colorvram(struct gfx_ctx *ctx,
	    uint32_t color);
extern	uint32_t newport_calc_hostrw_color(struct gfx_ctx *ctx,
	    uint32_t color);
extern	uint32_t newport_calc_colori_color(struct gfx_ctx *ctx,
	    uint32_t color);

extern	void newport_fill_rectangle_fast(struct gfx_ctx *dc, int x1, int y1,
	    int wi, int he, uint32_t color);

extern	void newport_fill_rectangle_setup(struct gfx_ctx *dc);
extern	void newport_fill_rectangle(struct gfx_ctx *dc, int x1, int y1,
	    int wi, int he, uint32_t color);

extern	bool newport_setup_hw(struct gfx_ctx *dc);

#endif	/* __NEWPORT_OPTS_H__ */
