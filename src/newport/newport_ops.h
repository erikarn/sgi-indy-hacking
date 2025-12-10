#ifndef	__NEWPORT_OPS_H__
#define	__NEWPORT_OPS_H__

extern	void rex3_wait_gfifo(struct gfx_ctx *dc);
extern	void rex3_wait_bfifo(struct gfx_ctx *dc);
extern	void vc2_write_ireg(struct gfx_ctx *dc, uint8_t ireg, uint16_t val);
extern	uint16_t vc2_read_ireg(struct gfx_ctx *dc, uint8_t ireg);

#endif	/* __NEWPORT_OPTS_H__ */
