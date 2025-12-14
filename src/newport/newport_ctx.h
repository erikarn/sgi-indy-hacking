#ifndef	__NEWPORT_CTX_H__
#define	__NEWPORT_CTX_H__

typedef enum {
	NewportBppModeUndefined = 0,

	/* Single buffered modes */
	NewportBppModeRgb8 = 1,
	NewportBppModeRgb12 = 2,
	NewportBppModeRgb24 = 3,
	NewportBppModeCi8 = 4,
} NewportBppMode;

typedef enum {
	NewportDoubleBufferNone = 0,
	NewportDoubleBufferA = 1,
	NewportDoubleBufferB = 2,
} NewportDoubleBufferMode;

struct gfx_ctx {
	int fd;
	void *addr;

	/* pixel clock in MHz, used for xmap9 programming */
	uint32_t cfreq;

	/* framebuffer/RAM pixel mode */
	NewportBppMode fb_mode;

	/* Incoming pixel mode (for HOSTRW, converting stuff, etc) */
	NewportBppMode pixel_mode;

	/* Double buffer configuration */
	/* Display buffer */
	NewportDoubleBufferMode display_buffer;
	/* Draw buffer */
	NewportDoubleBufferMode draw_buffer;

	bool log_regio;

	/* how many entries are in the FIFO */
	int gfifo_left;
};

#endif	/* __NEWPORT_CTX_H__ */
