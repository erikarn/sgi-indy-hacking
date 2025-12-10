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
#include "newport_ops.h"

struct newport_dcb_cs_params {
	int cs_width;
	int cs_hold;
	int cs_setup;
};

/* otherwise (likely PAL/NTSC modes) */
static struct newport_dcb_cs_params newport_dcb_cs_wslow = { 0, 12, 12 };
/* cfreq > 59MHz */
static struct newport_dcb_cs_params newport_dcb_cs_slow = { 0, 5, 5 };
/* cfreq > 119MHz */
static struct newport_dcb_cs_params newport_dcb_cs_fast = { 0, 1, 2 };

/*
 * Wait for the graphics FIFO and for it to be empty.
 *
 * This FIFO is used for submitted graphics register writes.
 */
void
rex3_wait_gfifo(struct gfx_ctx *dc)
{
	while (rex3_read(dc, REX3_REG_STATUS) &
	    (REX3_STATUS_GFXBUSY | REX3_STATUS_PIPELEVEL_MASK))
		;
}

/*
 * Wait for the backend FIFO and for it to be empty.
 *
 * This FIFO is used for speaking to the data backend, notably
 * the DCB.
 */
void
rex3_wait_bfifo(struct gfx_ctx *dc)
{
	while (rex3_read(dc, REX3_REG_STATUS) &
	    (REX3_STATUS_BACKBUSY | REX3_STATUS_BPIPELEVEL_MASK))
		;
}

void
vc2_write_ireg(struct gfx_ctx *dc, uint8_t ireg, uint16_t val)
{
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_3 |
	    REX3_DCBMODE_ENCRSINC |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_INDEX << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, (ireg << 24) | (val << 8));
}

uint16_t
vc2_read_ireg(struct gfx_ctx *dc, uint8_t ireg)
{
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_1 |
	    REX3_DCBMODE_ENCRSINC |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_INDEX << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, ireg << 24);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_2 |
	    REX3_DCBMODE_ENCRSINC |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_IREG << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	return (uint16_t)(rex3_read(dc, REX3_REG_DCBDATA0) >> 16);
}

#if 0
static uint16_t
vc2_read_ram(struct gfx_ctx *dc, uint16_t addr)
{
	vc2_write_ireg(dc, VC2_IREG_RAM_ADDRESS, addr);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_2 |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_RAM << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	return (uint16_t)(rex3_read(dc, REX3_REG_DCBDATA0) >> 16);
}

static void
vc2_write_ram(struct gfx_ctx *dc, uint16_t addr, uint16_t val)
{
	vc2_write_ireg(dc, VC2_IREG_RAM_ADDRESS, addr);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_2 |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_RAM << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, val << 16);
}
#endif

/*
 * Read from the given XMAP chip.
 *
 * Note this must not be used to read from NEWPORT_DCBADDR_XMAP_BOTH.
 */
u_int32_t
xmap9_read(struct gfx_ctx *dc, int chip, int crs)
{

	if (chip == NEWPORT_DCBADDR_XMAP_BOTH)
		chip = NEWPORT_DCBADDR_XMAP_0;

	rex3_write(dc, REX3_REG_DCBMODE,
		REX3_DCBMODE_DW_1 |
		(chip << REX3_DCBMODE_DCBADDR_SHIFT) |
		(crs << REX3_DCBMODE_DCBCRS_SHIFT) |
		(3 << REX3_DCBMODE_CSWIDTH_SHIFT) |
		(1 << REX3_DCBMODE_CSHOLD_SHIFT) |
		(2 << REX3_DCBMODE_CSSETUP_SHIFT));
	return (uint8_t) (rex3_read(dc, REX3_REG_DCBDATA0) >> 24);
}

/*
 * write to the given XMAP chip, or to both.
 */
void
xmap9_write(struct gfx_ctx *dc, int chip, int crs, uint8_t val)
{
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_1 |
	    (chip << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (crs << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (0 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (1 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (2 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, val << 24);
}

/*
 * Wait for the given XMAP9 mode FIFO to be ready.
 *
 * Since we're not trying to optimise for bursting mode writes,
 * just wait for /a/ FIFO entry to be avaliable.
 *
 * The XMAP9 documentation covers the gray mode encoding
 * of this register in case there's a future need for
 * bursting mode updates without individual FIFO checks.
 */
static inline void
xmap9_wait_chip(struct gfx_ctx *dc, int chip)
{
	while (xmap9_read(dc, chip, XMAP9_DCBCRS_FIFOAVAIL) == 0)
		;
}

/*
 * Wait for the backend FIFO and then both XMAP FIFOs to become available.
 */
static inline void
xmap9_wait(struct gfx_ctx *dc)
{
	rex3_wait_bfifo(dc);

	xmap9_wait_chip(dc, NEWPORT_DCBADDR_XMAP_0);
	xmap9_wait_chip(dc, NEWPORT_DCBADDR_XMAP_1);
}

uint32_t
xmap9_read_mode(struct gfx_ctx *dc, int chip, uint8_t idx)
{
	uint32_t mode = 0, val;
	int i;

	xmap9_wait_chip(dc, chip);

	for (i = 0; i < 4; i++) {
		xmap9_write(dc, chip, XMAP9_DCBCRS_MODE_SELECT,
		    (idx << 2) | i);
		val = xmap9_read(dc, chip, XMAP9_DCBCRS_MODE_SETUP);
		mode |= (val << (i * 8));
	}
	return (mode);
}

/*
 * Map the pixel clock frequency to which parameters to use for XMAP9
 * mode writes.
 */
static const struct newport_dcb_cs_params *
newport_hw_get_mode_cs_params(int cfreq)
{
	if (cfreq > 119)
		return &newport_dcb_cs_fast;
	if (cfreq > 59)
		return &newport_dcb_cs_slow;
	return &newport_dcb_cs_wslow;
}

/*
 * Write out the 32 bit mode entry to both XMAP9 chips.
 *
 * This is actually a fun clock domain crossing problem - the
 * XMAP9 isn't signaling an ACK back to the REX3 chip, so
 * the CS setup, width and hold times need to be calculated
 * based on 33MHz GIO clock (REX3) <-> the currently configured
 * pixel clock (XMAP9).
 *
 * The pixel clock is set by the firmware based on the attached
 * monitor; it programs the BT445 RAMDAC to generate a pixelclock
 * that's then divided in half and sent in both phases to the
 * various chips doing odd/even pixel handling (which includes the
 * XMAP9.)
 *
 * Unfortunately we don't have the pixel clock available to us - only
 * the monitor resolution from the VC2 table - so eventually the
 * driver will need to grow a way to read the monitor sense lines and
 * mirror what the firmware is doing.
 *
 * Also note on SGI Indy the PROM recognises "setenv monitor" to
 * force a 1280x1024x60Hz monitor (setenv monitor H) and
 * a 1280x1024*76Hz monitor (setenv monitor S).  To correctly handle
 * that we will ALSO need to parse the PROM environment and make
 * it available here.
 */
void
xmap9_write_mode(struct gfx_ctx *dc, uint32_t cfreq, uint8_t index,
    uint32_t mode)
{
	const struct newport_dcb_cs_params *cs;

	cs = newport_hw_get_mode_cs_params(cfreq);

	/* wait for FIFO if needed */
	xmap9_wait(dc);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_4 |
	    (NEWPORT_DCBADDR_XMAP_BOTH << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (XMAP9_DCBCRS_MODE_SETUP << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (cs->cs_width << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (cs->cs_hold << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (cs->cs_setup << REX3_DCBMODE_CSSETUP_SHIFT)
	);
	rex3_write(dc, REX3_REG_DCBDATA0, (index << 24) | (mode & 0xffffff));
}

#if 0

/**** Helper functions ****/
static void
newport_fill_rectangle(struct gfx_ctx *dc, int x1, int y1, int wi,
    int he, uint32_t color)
{
	int x2 = x1 + wi - 1;
	int y2 = y1 + he - 1;

	rex3_wait_gfifo(dc);
	
	rex3_write(dc, REX3_REG_DRAWMODE0, REX3_DRAWMODE0_OPCODE_DRAW |
	    REX3_DRAWMODE0_ADRMODE_BLOCK | REX3_DRAWMODE0_DOSETUP |
	    REX3_DRAWMODE0_STOPONX | REX3_DRAWMODE0_STOPONY);
	rex3_write(dc, REX3_REG_CLIPMODE, 0x1e00);
	rex3_write(dc, REX3_REG_DRAWMODE1,
	    REX3_DRAWMODE1_PLANES_RGB |
	    REX3_DRAWMODE1_DD_DD8 |
	    REX3_DRAWMODE1_RWPACKED |
	    REX3_DRAWMODE1_HD_HD8 |
	    REX3_DRAWMODE1_COMPARE_LT |
	    REX3_DRAWMODE1_COMPARE_EQ |
	    REX3_DRAWMODE1_COMPARE_GT |
	    REX3_DRAWMODE1_RGBMODE |
	    REX3_DRAWMODE1_FASTCLEAR |
	    REX3_DRAWMODE1_LO_SRC);
	rex3_write(dc, REX3_REG_WRMASK, 0xffffffff);
	rex3_write(dc, REX3_REG_COLORVRAM, color);
	rex3_write(dc, REX3_REG_XYSTARTI, (x1 << REX3_XYSTARTI_XSHIFT) | y1);

	rex3_write_go(dc, REX3_REG_XYENDI, (x2 << REX3_XYENDI_XSHIFT) | y2);
}

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

static void
newport_cmap_setrgb(struct gfx_ctx *dc, int index, uint8_t r,
    uint8_t g, uint8_t b)
{
	rex3_wait_bfifo(dc);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_2 |
	    REX3_DCBMODE_ENCRSINC |
	    (NEWPORT_DCBADDR_CMAP_BOTH << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (CMAP_DCBCRS_ADDRESS_LOW << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (1 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (1 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT) |
	    REX3_DCBMODE_SWAPENDIAN);

	rex3_write(dc, REX3_REG_DCBDATA0, index << 16);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_3 |
	    (NEWPORT_DCBADDR_CMAP_BOTH << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (CMAP_DCBCRS_PALETTE << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (1 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (1 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, (r << 24) + (g << 16) + (b << 8));
}

static void
newport_get_resolution(struct gfx_ctx *dc)
{
	uint16_t vep,lines;
	uint16_t linep,cols;
	uint16_t data;

	vep = vc2_read_ireg(dc, VC2_IREG_VIDEO_ENTRY);

	dc->dc_xres = 0;
	dc->dc_yres = 0;

	for (;;) {
		/* Iterate over runs in video timing table */

		cols = 0;

		linep = vc2_read_ram(dc, vep++);
		lines = vc2_read_ram(dc, vep++);

		if (lines == 0)
			break;

		do {
			/* Iterate over state runs in line sequence table */
		
			data = vc2_read_ram(dc, linep++);

			if ((data & 0x0001) == 0)
				cols += (data >> 7) & 0xfe;

			if ((data & 0x0080) == 0)
				data = vc2_read_ram(dc, linep++);
		} while ((data & 0x8000) == 0);

		if (cols != 0) {
			if (cols > dc->dc_xres)
				dc->dc_xres = cols;

			dc->dc_yres += lines;
		}
	}
}

/*
 * Read from an 8 bit CMAP register.
 *
 * This must only be used on individual CMAPs (0 or 1), not both.
 */
static uint8_t
cmap_reg_read(struct gfx_ctx *dc, uint8_t id, uint8_t reg)
{
	if (id == NEWPORT_DCBADDR_CMAP_BOTH)
		id = NEWPORT_DCBADDR_CMAP_0;

	/* Get various revisions */
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_1 |
	    (id << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (reg << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (1 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (1 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));
	return (uint8_t)(rex3_read(dc, REX3_REG_DCBDATA0) >> 24);
}

static void
newport_probe_monitor(struct gfx_ctx *dc)
{
	const char *m;
	uint8_t scratch;

	/*
	 * CMAP1 - the 4 monitor sense bits are on bits 7:4.
	 *
	 * Note that for MACH_SGI_IP22_GUINNESS (Indy), the monitor PROM
	 * variable can override a 'default' monitor setting of 1024x768
	 * with two others - H = 1280x1024x60Hz and S = 1280x1024x76Hz.
	 * So when (eventually) doing a monitor ID lookup we'll also
	 * need to handle that.
	 */
	scratch = cmap_reg_read(dc, NEWPORT_DCBADDR_CMAP_1,
	    CMAP_DCBCRS_REVISION);
	aprint_debug("%s: CMAP_1 REVISION 0x%02x\n", __func__, scratch);
	dc->dc_monitor_cmap_id = (scratch >> 4) & 0x0f;
	dc->dc_monitor_prom_id = -1;

	/* Only check the PROM monitor on SGI Indy */
	if (mach_subtype != MACH_SGI_IP22_GUINNESS)
		return;

	m = arcbios_GetEnvironmentVariable("monitor");
	if (m == NULL)
		return;
	else if (m[0] == '0')
		return;

	/*
	 * H = 1280x1024, 60Hz
	 * S = 1280x1024, 76Hz
	 */
	if (m[0] == 'h' || m[0] == 'H')
		dc->dc_monitor_prom_id = 10;
	else if (m[0] == 'S')
		dc->dc_monitor_prom_id = 1;
}

/*
 * Probe the hardware as handed to us by the boot firmware
 * before it's potentially fiddled with by the console and
 * X11 servers.
 */
static void
newport_probe_hw(struct gfx_ctx *dc)
{
	uint32_t scratch;

	/* Get various revisions */

	/*
	 * CMAP0 - CMAP revision bits 3:0, board revision bits 6:4,
	 * If boardrev > 1 then the b7 == 1 signals an 8 bit framebuffer.
	 */
	rex3_wait_bfifo(dc);
	scratch = cmap_reg_read(dc, NEWPORT_DCBADDR_CMAP_0,
	    CMAP_DCBCRS_REVISION);
	aprint_debug("%s: CMAP_0 REVISION 0x%02x\n", __func__, scratch);

	dc->dc_boardrev = (scratch >> 4) & 0x07;
	dc->dc_cmaprev = scratch & 0x07;

	rex3_wait_bfifo(dc);
	dc->dc_xmaprev = xmap9_read(dc, NEWPORT_DCBADDR_XMAP_0,
	    XMAP9_DCBCRS_REVISION) & 0x07;
	dc->dc_depth = ( (dc->dc_boardrev > 1) && (scratch & 0x80)) ? 8 : 24;

	aprint_debug("%s: XMAP_0 REVISION: 0x%08x\n", __func__,
	    xmap9_read(dc, NEWPORT_DCBADDR_XMAP_0, XMAP9_DCBCRS_REVISION));

	scratch = vc2_read_ireg(dc, VC2_IREG_CONFIG);
	dc->dc_vc2rev = (scratch & VC2_IREG_CONFIG_REVISION) >> 5;
	aprint_debug("%s: VC2_IREG_CONFIG config: 0x%04x\n", __func__, scratch);
}

/*
 * Adjust the XMAP configuration based on the earlier probed board type.
 *
 * This isn't the actual operating mode; this is instead the underlying
 * hardware type.
 */
static void
newport_hw_adjust_xmap_cfg(struct gfx_ctx *dc, uint8_t *dcbcfg)
{

	/* Configure 8 or 24 bit hardware */
	if (dc->dc_depth == 8)
		*dcbcfg |= XMAP9_CONFIG_8BIT_SYSTEM;
	else
		*dcbcfg &= ~XMAP9_CONFIG_8BIT_SYSTEM;

	/* XXX TODO: we aren't using PUP, so disable it? */

	/*
	 * Note: we're leaving the event/odd, fast/slow pclk,
	 * video option board config and colourmap alone.
	 */
}

static void
newport_setup_hw_ci_cmap(struct gfx_ctx *dc)
{
	int i;
	uint8_t ctmp;

	for (i = 0; i < 256; i++) {
		uint8_t our_cmap[768];

		ctmp = i & 0xe0;
		/*
		 * replicate bits so 0xe0 maps to a red value of 0xff
		 * in order to make white look actually white
		 */
		ctmp |= (ctmp >> 3) | (ctmp >> 6);
		our_cmap[i * 3] = ctmp;

		ctmp = (i & 0x1c) << 3;
		ctmp |= (ctmp >> 3) | (ctmp >> 6);
		our_cmap[i * 3 + 1] = ctmp;

		ctmp = (i & 0x03) << 6;
		ctmp |= ctmp >> 2;
		ctmp |= ctmp >> 4;
		our_cmap[i * 3 + 2] = ctmp;

		newport_cmap_setrgb(dc, i, our_cmap[i * 3],
		    our_cmap[i * 3 + 1], our_cmap[i * 3 + 2]);
	}
}

static void
newport_setup_hw_rgb2_cmap(struct gfx_ctx *dc)
{
	int i;

	for (i = 0; i < 256; i++)
		newport_cmap_setrgb(dc, 0x1f00 + i, i, i, i);
}

/*
 * Setup the XMAP9 mode registers with the given mode.
 *
 * This tells the two XMAP9s (one even column, one odd column)
 * how to interpret the pixel data being streamed in from framebuffer
 * memory and what config bits to expose to the CMAP hardware and
 * the DAC as it's converted into final RGB signals for display.
 *
 * Each entry in the mode table is a DID (display ID) in the VC2
 * chip, and the VC2 chip will shift out a DID value to pair with
 * the framebuffer memory contents being fed into the XMAP9s.
 * Since we're not currently filling the VC2 DID table with values,
 * just program them all in here with the same configuration.
 */
static void
newport_setup_hw_xmap9_modes(struct gfx_ctx *dc,
    uint32_t mode_mask)
{
	int i;

	for (i = 0; i < 32; i++) {
		xmap9_write_mode(dc, i, mode_mask);
	}
	rex3_wait_bfifo(dc);
	xmap9_write(dc, NEWPORT_DCBADDR_XMAP_BOTH,
	    XMAP9_DCBCRS_MODE_SELECT, 0);
}

static void
newport_setup_hw(struct gfx_ctx *dc, int depth)
{
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

	/* Setup XMAP9s */
	rex3_wait_bfifo(dc);
	xmap9_write(dc, NEWPORT_DCBADDR_XMAP_BOTH, XMAP9_DCBCRS_CURSOR_CMAP,
	    0);

	/*
	 * Always configure the 8 bit or 24 bit configuration regardless
	 * of the desired bit depth.  The XMAP9 documentation mentions this
	 * does have some behaviour changes regardless of the pixel formats
	 * being used.
	 */
	rex3_wait_bfifo(dc);
	dcbcfg = xmap9_read(dc, NEWPORT_DCBADDR_XMAP_0, XMAP9_DCBCRS_CONFIG);
	newport_hw_adjust_xmap_cfg(dc, &dcbcfg);
	rex3_wait_bfifo(dc);
	xmap9_write(dc, NEWPORT_DCBADDR_XMAP_0, XMAP9_DCBCRS_CONFIG, dcbcfg);
	aprint_debug("%s: XMAP_0 config: 0x%02x\n", __func__, dcbcfg);

	dcbcfg = xmap9_read(dc, NEWPORT_DCBADDR_XMAP_1, XMAP9_DCBCRS_CONFIG);
	newport_hw_adjust_xmap_cfg(dc, &dcbcfg);
	rex3_wait_bfifo(dc);
	xmap9_write(dc, NEWPORT_DCBADDR_XMAP_1, XMAP9_DCBCRS_CONFIG, dcbcfg);
	aprint_debug("%s: XMAP_1 config: 0x%02x\n", __func__, dcbcfg);

	if (depth == 8) {
		/*
		 * Configure an 8 bit RGB colour map that uses the netbsd
		 * packed RGB 332 format.  The rendering routines use these
		 * values from the raster map attribute list.
		 */
		newport_setup_hw_xmap9_modes(dc, XMAP9_MODE_GAMMA_BYPASS |
		    XMAP9_MODE_PIXSIZE_8BPP | XMAP9_MODE_PIXMODE_CI);
	} else {
		/*
		 * Configure the hardware to use the a 24 bit RGB table at
		 * RGB2 in CMAP.
		 */
		newport_setup_hw_xmap9_modes(dc, XMAP9_MODE_GAMMA_BYPASS |
		    XMAP9_MODE_PIXSIZE_24BPP | XMAP9_MODE_PIXMODE_RGB2);
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
	newport_setup_hw_ci_cmap(dc);

	/*
	 * Write a ramp into RGB2 CMAP.
	 *
	 * This won't be used in 8 bit console mode as that is using
	 * CI pixels, not RGB8 pixels.  For 8 bit framebuffer mode it'll
	 * either use the RGB332 table programmed into CMAP CI table 0
	 * or X11 will allocate private colourmaps as needed.
	 */
	newport_setup_hw_rgb2_cmap(dc);
}

#endif
