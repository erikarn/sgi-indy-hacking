#ifndef	__NEWPORT_REGIO_H__
#define	__NEWPORT_REGIO_H__

extern	uint32_t rex3_read(struct gfx_ctx *ctx, uint32_t rexreg);
extern	void rex3_write(struct gfx_ctx *ctx, uint32_t rexreg, uint32_t val);
extern	void rex3_write_go(struct gfx_ctx *ctx, uint32_t rexreg, uint32_t val);

#endif	/* __NEWPORT_REGIO_H__ */
