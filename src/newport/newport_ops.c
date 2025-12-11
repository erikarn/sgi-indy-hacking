#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <strings.h>
#include <dev/wscons/wsconsio.h>
#include <err.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include "newport_regs.h"
#include "newport_ctx.h"
#include "newport_regio.h"
#include "newport_hwops.h"
#include "newport_ops.h"

/*
 * Determine the DRAWMODE1 configuration to use.
 *
 * This calculates which drawplanes to use, which mode to put
 * the drawer to use, the input and output pixel formats, etc.
 */
uint32_t
newport_calc_drawmode1(struct gfx_ctx *ctx)
{
	switch (ctx->pixel_mode) {
	case NewportBppModeRgb24:	/* input pixels are RGB888 */
		if (ctx->fb_mode == NewportBppModeRgb24) { /* FB pixels are rgb888 */
			return
			    REX3_DRAWMODE1_DD_DD24 |
			    REX3_DRAWMODE1_RWPACKED |
			    REX3_DRAWMODE1_RGBMODE |
			    REX3_DRAWMODE1_HD_HD24;

		} else if (ctx->fb_mode == NewportBppModeRgb8) { /* FB pixels are rgb8 */
			/* XXX TODO: make DITHER a flag? */
			return
			    REX3_DRAWMODE1_DD_DD8 |
			    REX3_DRAWMODE1_RWPACKED |
			    REX3_DRAWMODE1_RGBMODE |
			    REX3_DRAWMODE1_DITHER |
			    REX3_DRAWMODE1_HD_HD24;
		} else {
			goto unknown;
		}
	case NewportBppModeCi8:
		/* Always just assume input/output is 8 bit for now */
		return
		    REX3_DRAWMODE1_DD_DD8 |
		    REX3_DRAWMODE1_RWPACKED |
		    REX3_DRAWMODE1_HD_HD8;

	case NewportBppModeRgb8:
	case NewportBppModeRgb12:
		/* These two are unimplemented */
	case NewportBppModeUndefined:
		goto unknown;
	}
unknown:
		printf("%s: called on unimplemented fbmode (%d) pixmode (%d)\n",
		    __func__, ctx->fb_mode, ctx->pixel_mode);
		return
		    REX3_DRAWMODE1_DD_DD8 |
		    REX3_DRAWMODE1_RWPACKED |
		    REX3_DRAWMODE1_HD_HD8;
}

/*
 * Determine the WRMODE to use.
 *
 * This takes into account the requested drawplanes and will
 * remap them appropriately.
 *
 * TODO: actually do the plane stuff!
 */
uint32_t
newport_calc_wrmode(struct gfx_ctx *ctx, uint32_t planemask)
{
	/*
	 * TODO: for now don't do plane mask stuff, always write to all
	 * planes.
	 *
	 * TODO: obviously this doesn't know yet about the overlay/underlay
	 * stuff, double buffered stuff, etc, etc.
	 */
	/* For now only support ci8 in/out */
	switch (ctx->fb_mode) {
	case NewportBppModeRgb8:
		return (0xff);
	case NewportBppModeRgb12:
		return (0xfff);
	case NewportBppModeRgb24:
		return (0xffffff);
	case NewportBppModeCi8:
		return (0xff);
	case NewportBppModeUndefined:
		printf("%s: called on Undefined\n", __func__);
		return (0xffffff);
	}
	return (0xffffff);
}

/**
 * Map 24 bit RGB pixel to 24-bit RGB framebuffer format.
 *
 * This requires quite a bit of swizzling!
 */
uint32_t
newport_calc_rgb888_to_fb_rgb888(uint32_t color)
{
	unsigned int res;
	unsigned int i;
	unsigned int mr, mg, mb;
	unsigned int sr, sg, sb;

	res = 0;

	/* Input here is RGB */
	mr = 0x800000;
	mg = 0x008000;
	mb = 0x000080;

	sr = 2;
	sg = 1;
	sb = 4;

	for (i = 0; i < 8; i++) {
		res |= (color & mr)?sr:0;
		res |= (color & mg)?sg:0;
		res |= (color & mb)?sb:0;

		sr <<= 3;
		sg <<= 3;
		sb <<= 3;
		mr >>= 1;
		mg >>= 1;
		mb >>= 1;
	}

	return res;
}

/**
 * Map 24 bit RGB pixel to 8 bit RGB framebuffer format.
 *
 * This is the same as the 24 bit format, but truncated
 * to 8 bits, which correctly captures the high 2/3 bits.
 */
uint32_t
newport_calc_rgb888_to_fb_rgb332(uint32_t color)
{
	return newport_calc_rgb888_to_fb_rgb888(color) & 0xff;
}

/**
 * Map 24 bit RGB pixel format to the HOSTRW format (BGR).
 *
 * TODO: really should include the alpha channel at some point..
 */
uint32_t
newport_calc_rgb888_to_bgr888(uint32_t color)
{
	return  ((color & 0x0000FF) << 16)
	    | ((color & 0xFF0000) >> 16)
	    | ((color & 0x00FF00));
}

/*
 * Calculate the colour to use for COLORVRAM.
 *
 * The input is either RGB888, RGB332 or CI8.
 *
 * This uses the raw output pixel format, not the HOSTRW
 * input pixel format or (I think) the COLORI RGB layout.
 * It's only 1:1 for CI modes.
 */
uint32_t
newport_calc_colorvram(struct gfx_ctx *ctx, uint32_t color)
{

	switch (ctx->fb_mode) {
	case NewportBppModeCi8:	/* output is ci8, assume ci8 in */
		return (color & 0xff);
	case NewportBppModeRgb24:	/* output is rgb24, assume rgb24 in for now */
		return newport_calc_rgb888_to_fb_rgb888(color);
	case NewportBppModeRgb8:
		/* Output is rgb8, input is either rgb8 or rgb24 */
		if (ctx->pixel_mode == NewportBppModeRgb24) {
			return newport_calc_rgb888_to_fb_rgb332(color);
		} else {
			break;
		}
	default:
		break;
	}

	printf("%s: pixel (%d) -> output (%d) is unsupported\n",
	    __func__, ctx->pixel_mode, ctx->fb_mode);
	return (color);
}

uint32_t
newport_calc_hostrw_color(struct gfx_ctx *ctx, uint32_t color)
{
	/* XXX for now */
	return (color);
}

/**
 * Turn the input colour into our desired colour for use in
 * the COLORI register.
 *
 * The COLORI register is either CI or BGR888.
 *
 * So for CI it's just returned as-is, but for 8/12 bit colour
 * it will need to be expanded out to RGB888 and then converted
 * to BGR888.
 */
uint32_t
newport_calc_colori_color(struct gfx_ctx *ctx, uint32_t color)
{
	switch (ctx->fb_mode) {
	case NewportBppModeCi8:	/* output is ci8, assume ci8 in */
		return (color & 0xff);
	case NewportBppModeRgb24:	/* output is rgb24, assume rgb24 in for now */
		return newport_calc_rgb888_to_bgr888(color);
	case NewportBppModeRgb8:
		/* Output is bgr888, only handle rgb24 input for now */
		if (ctx->pixel_mode == NewportBppModeRgb24) {
			return newport_calc_rgb888_to_bgr888(color);
		} else {
			break;
		}
	default:
		break;
	}

	printf("%s: pixel (%d) -> output (%d) is unsupported\n",
	    __func__, ctx->pixel_mode, ctx->fb_mode);
	return (color);
}

/**
 * Solid fill a rectangle with the given color value.
 *
 * This doesn't do dithering or alpha blending or anything;
 * it's a stright up fast fill.
 */
void
newport_fill_rectangle_fast(struct gfx_ctx *dc, int x1, int y1, int wi,
    int he, uint32_t color)
{
	uint32_t drawmode1;

	int x2 = x1 + wi - 1;
	int y2 = y1 + he - 1;

//	dc->log_regio = true;

	drawmode1 = newport_calc_drawmode1(dc);

	rex3_wait_gfifo(dc);

	rex3_write(dc, REX3_REG_DRAWMODE0, REX3_DRAWMODE0_OPCODE_DRAW |
	    REX3_DRAWMODE0_ADRMODE_BLOCK | REX3_DRAWMODE0_DOSETUP |
	    REX3_DRAWMODE0_STOPONX | REX3_DRAWMODE0_STOPONY);
	rex3_write(dc, REX3_REG_CLIPMODE, 0x1e00);
	rex3_write(dc, REX3_REG_DRAWMODE1,
	    drawmode1 |
	    REX3_DRAWMODE1_PLANES_RGB |
	    REX3_DRAWMODE1_COMPARE_LT |
	    REX3_DRAWMODE1_COMPARE_EQ |
	    REX3_DRAWMODE1_COMPARE_GT |
	    REX3_DRAWMODE1_FASTCLEAR |
	    REX3_DRAWMODE1_LO_SRC);
	rex3_write(dc, REX3_REG_WRMASK, newport_calc_wrmode(dc, 0xffffffff));
	rex3_write(dc, REX3_REG_COLORVRAM, newport_calc_colorvram(dc, color));
	rex3_write(dc, REX3_REG_XYSTARTI, (x1 << REX3_XYSTARTI_XSHIFT) | y1);

	rex3_write_go(dc, REX3_REG_XYENDI, (x2 << REX3_XYENDI_XSHIFT) | y2);
	dc->log_regio = false;
}

/**
 * Solid fill a rectangle with the given color value.
 */
void
newport_fill_rectangle(struct gfx_ctx *dc, int x1, int y1, int wi,
    int he, uint32_t color)
{
	uint32_t drawmode1;

	int x2 = x1 + wi - 1;
	int y2 = y1 + he - 1;

//	dc->log_regio = true;

	drawmode1 = newport_calc_drawmode1(dc);

	rex3_wait_gfifo(dc);

	rex3_write(dc, REX3_REG_DRAWMODE0, REX3_DRAWMODE0_OPCODE_DRAW |
	    REX3_DRAWMODE0_ADRMODE_BLOCK | REX3_DRAWMODE0_DOSETUP |
	    REX3_DRAWMODE0_STOPONX | REX3_DRAWMODE0_STOPONY);
	rex3_write(dc, REX3_REG_CLIPMODE, 0x1e00);
	rex3_write(dc, REX3_REG_DRAWMODE1,
	    drawmode1 |
	    REX3_DRAWMODE1_PLANES_RGB |
	    REX3_DRAWMODE1_COMPARE_LT |
	    REX3_DRAWMODE1_COMPARE_EQ |
	    REX3_DRAWMODE1_COMPARE_GT |
	    REX3_DRAWMODE1_LO_SRC);
	rex3_write(dc, REX3_REG_WRMASK, newport_calc_wrmode(dc, 0xffffffff));
	rex3_write(dc, REX3_REG_COLORI, newport_calc_colori_color(dc, color));
	rex3_write(dc, REX3_REG_XYSTARTI, (x1 << REX3_XYSTARTI_XSHIFT) | y1);

	rex3_write_go(dc, REX3_REG_XYENDI, (x2 << REX3_XYENDI_XSHIFT) | y2);
	dc->log_regio = false;
}

#if 0

static void
newport_bitblt(struct gfx_ctx *dc, int xs, int ys, int xd,
    int yd, int wi, int he, int rop)
{
	int xe, ye;
	uint32_t tmp;

	rex3_wait_gfifo(dc);
	if (yd > ys) {
		/* need to copy bottom up */
		ye = ys;
		yd += he - 1;
		ys += he - 1;
	} else
		ye = ys + he - 1;

	if (xd > xs) {
		/* need to copy right to left */
		xe = xs;
		xd += wi - 1;
		xs += wi - 1;
	} else
		xe = xs + wi - 1;

	rex3_write(dc, REX3_REG_DRAWMODE0, REX3_DRAWMODE0_OPCODE_SCR2SCR |
	    REX3_DRAWMODE0_ADRMODE_BLOCK | REX3_DRAWMODE0_DOSETUP |
	    REX3_DRAWMODE0_STOPONX | REX3_DRAWMODE0_STOPONY);
	rex3_write(dc, REX3_REG_DRAWMODE1,
	    REX3_DRAWMODE1_PLANES_CI |
	    REX3_DRAWMODE1_DD_DD8 |
	    REX3_DRAWMODE1_RWPACKED |
	    REX3_DRAWMODE1_HD_HD8 |
	    REX3_DRAWMODE1_COMPARE_LT |
	    REX3_DRAWMODE1_COMPARE_EQ |
	    REX3_DRAWMODE1_COMPARE_GT |
	    ((rop << 28) & REX3_DRAWMODE1_LOGICOP_MASK));
	rex3_write(dc, REX3_REG_XYSTARTI, (xs << REX3_XYSTARTI_XSHIFT) | ys);
	rex3_write(dc, REX3_REG_XYENDI, (xe << REX3_XYENDI_XSHIFT) | ye);

	tmp = (yd - ys) & 0xffff;
	tmp |= (xd - xs) << REX3_XYMOVE_XSHIFT;

	rex3_write_go(dc, REX3_REG_XYMOVE, tmp);
}
#endif

bool
newport_setup_hw(struct gfx_ctx *dc)
{
#if 0
	uint16_t __unused(curp), tmp;
	uint8_t dcbcfg;

	/* Setup cursor glyph */
	curp = vc2_read_ireg(dc, VC2_IREG_CURSOR_ENTRY);

	/* Setup VC2 to a known state */
	tmp = vc2_read_ireg(dc, VC2_IREG_CONTROL) & VC2_CONTROL_INTERLACE;
	vc2_write_ireg(dc, VC2_IREG_CONTROL, tmp |
	    VC2_CONTROL_DISPLAY_ENABLE |
	    VC2_CONTROL_VTIMING_ENABLE |
	    VC2_CONTROL_DID_ENABLE |
	    VC2_CONTROL_CURSORFUNC_ENABLE /*|
	    VC2_CONTROL_CURSOR_ENABLE*/);

	/* disable all clipping */
	rex3_write(dc, REX3_REG_CLIPMODE, 0x1e00);
#endif

	rex3_wait_bfifo(dc);

	/* Set cursor to use CMAP0 */
	xmap9_write(dc, NEWPORT_DCBADDR_XMAP_BOTH, XMAP9_DCBCRS_CURSOR_CMAP,
	    0);

	switch (dc->fb_mode) {
	case NewportBppModeRgb8:
		printf("%s: Configuring output 8 bit RGB\n", __func__);
		/*
		 * Configure the hardware to use the a 24 bit RGB table at
		 * RGB2 in CMAP.
		 */
		newport_setup_hw_xmap9_modes(dc, XMAP9_MODE_GAMMA_BYPASS |
		    XMAP9_MODE_PIXSIZE_8BPP | XMAP9_MODE_PIXMODE_RGB2);
		break;
	case NewportBppModeRgb24:
		printf("%s: Configuring output 24 bit RGB\n", __func__);
		/*
		 * Configure the hardware to use the a 24 bit RGB table at
		 * RGB2 in CMAP.
		 */
		newport_setup_hw_xmap9_modes(dc, XMAP9_MODE_GAMMA_BYPASS |
		    XMAP9_MODE_PIXSIZE_24BPP | XMAP9_MODE_PIXMODE_RGB2);
		break;
	case NewportBppModeCi8:
		printf("%s: Configuring output 8 bit CI\n", __func__);
		/*
		 * Configure an 8 bit RGB colour map that uses the netbsd
		 * packed RGB 332 format.  The rendering routines use these
		 * values from the raster map attribute list.
		 */
		newport_setup_hw_xmap9_modes(dc, XMAP9_MODE_GAMMA_BYPASS |
		    XMAP9_MODE_PIXSIZE_8BPP | XMAP9_MODE_PIXMODE_CI);
		break;
	default:
		printf("%s: unsupported FB mode (%d)\n", __func__,
		    dc->fb_mode);
		return false;
	}

	/* Setup REX3 */
	rex3_write(dc, REX3_REG_XYWIN, (4096 << 16) | 4096);
	rex3_write(dc, REX3_REG_TOPSCAN, 0x3ff); /* XXX Why? XXX */

	/*
	 * Setup CMAP CI table 0 for an RGB 332 packing.
	 *
	 * This will be used by 8 bit RGB pixel modes so we can use
	 * RGB 332 and not have to teach the NetBSD console code or
	 * this driver to map RGB 332 -> the Newport interlaced RGB8 format.
	 */
	newport_setup_hw_ci_cmap(dc, 0);

	/*
	 * Write a ramp into RGB2 CMAP.
	 *
	 * This won't be used in 8 bit console mode as that is using
	 * CI pixels, not RGB8 pixels.  For 8 bit framebuffer mode it'll
	 * either use the RGB332 table programmed into CMAP CI table 0
	 * or X11 will allocate private colourmaps as needed.
	 */
	newport_setup_hw_rgb2_cmap(dc);

	return true;
}
