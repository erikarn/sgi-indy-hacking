#ifndef	__NEWPORT_OPS_H__
#define	__NEWPORT_OPS_H__

extern	void rex3_wait_gfifo(struct gfx_ctx *dc);
extern	void rex3_wait_bfifo(struct gfx_ctx *dc);
extern	void vc2_write_ireg(struct gfx_ctx *dc, uint8_t ireg, uint16_t val);
extern	uint16_t vc2_read_ireg(struct gfx_ctx *dc, uint8_t ireg);

extern	u_int32_t xmap9_read(struct gfx_ctx *dc, int chip, int crs);
extern	void xmap9_write(struct gfx_ctx *dc, int chip, int crs, uint8_t val);
extern	uint32_t xmap9_read_mode(struct gfx_ctx *dc, int chip, uint8_t idx);
extern	void xmap9_write_mode(struct gfx_ctx *dc, uint32_t cfreq,
	    uint8_t index, uint32_t mode);

#endif	/* __NEWPORT_OPTS_H__ */
