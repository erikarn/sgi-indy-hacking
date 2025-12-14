#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <strings.h>
#include <dev/wscons/wsconsio.h>
#include <err.h>
#include <sched.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include "newport_regs.h"
#include "newport_ctx.h"
#include "newport_regio.h"
#include "newport_hwops.h"

/*
 * These are the hardware operation calls for the various component
 * of the newport graphics hardware.
 */

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
 *
 * nentries reserves that many entries in the FIFO tracking.
 */
void
rex3_wait_gfifo(struct gfx_ctx *dc, int nentries)
{
	uint32_t fifo_level, reg;

	/* Ensure we don't shoot past the fifo depth */
	if (nentries > NEWPORT_GFIFO_ENTRIES)
		nentries = NEWPORT_GFIFO_ENTRIES;

	/* If we have enough slots left, then return it */
	if (nentries <= dc->gfifo_left) {
		dc->gfifo_left -= nentries;
		return;
	}

	/* Wait until we have enough slots for this request */
	while (true) {
		reg = rex3_read(dc, REX3_REG_STATUS);
		if ((reg & REX3_STATUS_GFXBUSY) == 0) {
			dc->gfifo_left = NEWPORT_GFIFO_ENTRIES - nentries;
			break;
		}

		/*
		 * GFXBUSY is set; the GFIFOLEVEL register is valid
		 * GFIFOLEVEL is how many entries are active in the FIFO, so
		 * we need to subtract it from GFIFOENTRIES to see what's left.
		 */
		fifo_level = NEWPORT_GFIFO_ENTRIES - ((reg >> 7) & 0x3f);
		if (fifo_level >= nentries) {
			dc->gfifo_left = fifo_level - nentries;
			break;
		}
		/* XXX ew */
		sched_yield();
	}
}

/*
 * Fully wait for the graphics FIFO to be idle and then
 * reserve some FIFO slots.
 */
void
rex3_wait_gfifo_idle(struct gfx_ctx *dc, int nentries)
{
	while (rex3_read(dc, REX3_REG_STATUS) &
	    (REX3_STATUS_GFXBUSY | REX3_STATUS_PIPELEVEL_MASK))
		;
	dc->gfifo_left = NEWPORT_GFIFO_ENTRIES - nentries;
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
xmap9_write_mode(struct gfx_ctx *dc, uint8_t index, uint32_t mode)
{
	const struct newport_dcb_cs_params *cs;

	cs = newport_hw_get_mode_cs_params(dc->cfreq);

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

void
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

/*
 * Read from an 8 bit CMAP register.
 *
 * This must only be used on individual CMAPs (0 or 1), not both.
 */
uint8_t
cmap_reg_read(struct gfx_ctx *dc, uint8_t id, uint8_t reg)
{
	if (id == NEWPORT_DCBADDR_CMAP_BOTH)
		id = NEWPORT_DCBADDR_CMAP_0;

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_1 |
	    (id << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (reg << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (1 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (1 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));
	return (uint8_t)(rex3_read(dc, REX3_REG_DCBDATA0) >> 24);
}

/*
 * Setup an RGB332 indexed colourmap at the given CMAP base.
 */
void
newport_setup_hw_ci_cmap(struct gfx_ctx *dc, uint32_t base)
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

		newport_cmap_setrgb(dc, i + base, our_cmap[i * 3],
		    our_cmap[i * 3 + 1], our_cmap[i * 3 + 2]);
	}
}

/*
 * Setup an RGB lookup table at RGB2.
 */
void
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
void
newport_setup_hw_xmap9_modes(struct gfx_ctx *dc, uint32_t mode_mask)
{
	int i;

	for (i = 0; i < 32; i++) {
		xmap9_write_mode(dc, i, mode_mask);
	}
	rex3_wait_bfifo(dc);
	xmap9_write(dc, NEWPORT_DCBADDR_XMAP_BOTH,
	    XMAP9_DCBCRS_MODE_SELECT, 0);
}
