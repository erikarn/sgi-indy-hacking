#ifndef	__NEWPORT_HWOPS_H__
#define	__NEWPORT_HWOPS_H__

extern	void rex3_wait_gfifo(struct gfx_ctx *dc, int nentries);
extern	void rex3_wait_gfifo_idle(struct gfx_ctx *dc, int nentries);
extern	void rex3_wait_bfifo(struct gfx_ctx *dc);
extern	void vc2_write_ireg(struct gfx_ctx *dc, uint8_t ireg, uint16_t val);
extern	uint16_t vc2_read_ireg(struct gfx_ctx *dc, uint8_t ireg);

extern	u_int32_t xmap9_read(struct gfx_ctx *dc, int chip, int crs);
extern	void xmap9_write(struct gfx_ctx *dc, int chip, int crs, uint8_t val);
extern	uint32_t xmap9_read_mode(struct gfx_ctx *dc, int chip, uint8_t idx);
extern	void xmap9_write_mode(struct gfx_ctx *dc, uint8_t index,
	    uint32_t mode);
extern	void newport_cmap_setrgb(struct gfx_ctx *dc, int index, uint8_t r,
	    uint8_t g, uint8_t b);
extern	uint8_t cmap_reg_read(struct gfx_ctx *dc, uint8_t id, uint8_t reg);
extern	void newport_setup_hw_ci_cmap(struct gfx_ctx *dc, uint32_t base);
extern	void newport_setup_hw_rgb2_cmap(struct gfx_ctx *dc);
extern	void newport_setup_hw_xmap9_modes(struct gfx_ctx *dc,
	    uint32_t mode_mask);

#endif	/* __NEWPORT_OPTS_H__ */
