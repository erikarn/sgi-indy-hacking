#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <strings.h>
#include <dev/wscons/wsconsio.h>
#include <err.h>
#include <time.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include "newport_regs.h"
#include "newport_ctx.h"
#include "newport_regio.h"
#include "newport_hwops.h"
#include "newport_ops.h"

static bool
verify_newport(void)
{
	int fd, type, i;

	fd = open("/dev/ttyE0", O_RDONLY, 0);
	i = ioctl(fd, WSDISPLAYIO_GTYPE, &type);
	close(fd);

	return ((i == 0) && (type == WSDISPLAY_TYPE_NEWPORT));
}

static void
gfx_ctx_init(struct gfx_ctx *ctx)
{
	bzero(ctx, sizeof(*ctx));
	ctx->fd = -1;
	ctx->addr = NULL;

	/* Defaults */
	ctx->fb_mode = NewportBppModeCi8;
	ctx->pixel_mode = NewportBppModeCi8;
	ctx->cfreq = 70; /* 1024x768 60Hz */
}

static bool
newport_open(struct gfx_ctx *ctx)
{
	int i, type;

	ctx->fd = open("/dev/ttyE0", O_RDWR, 0);
	if (ctx->fd < 0) {
		warn("%s: couldn't open /dev/ttyE0", __func__);
		goto error;
	}

	/* TODO: mapped or dumbfb? */
	type = WSDISPLAYIO_MODE_MAPPED;
	i = ioctl(ctx->fd, WSDISPLAYIO_SMODE, &type);
	if (i != 0) {
		warn("%s: WSDISPLAYIO_SMODE", __func__);
		goto error;
	}

	/* mmap() */
	ctx->addr = mmap(NULL, NEWPORT_IOSPACE_SIZE, PROT_READ | PROT_WRITE,
	    0, ctx->fd, NEWPORT_IOSPACE_START);
	if (ctx->addr == MAP_FAILED) {
		ctx->addr = NULL;
		warn("%s: mmap()", __func__);
		goto error;
	}

	return true;

error:
	if (ctx->fd != -1)
		close(ctx->fd);
	return false;
}

static void
newport_close(struct gfx_ctx *ctx)
{
	int i, type;

	if (ctx->addr != NULL) {
		munmap(ctx->addr, NEWPORT_IOSPACE_SIZE);
		ctx->addr = NULL;
	}

	/* XXX i know */
	if (ctx->fd > 0) {
		/* Reset */
		type = WSDISPLAYIO_MODE_EMUL;
		i = ioctl(ctx->fd, WSDISPLAYIO_SMODE, &type);
		if (i != 0)
			warn("%s: couldn't restore console mode", __func__);

		close(ctx->fd);
		ctx->fd = -1;
	}
}

static void
benchmark_rectangle(struct gfx_ctx *ctx, int tw, int th, int tcount)
{
	struct timespec ts_start, ts_end;
	uint64_t ts;
	float fills_per_sec, pixels_per_sec;

	newport_fill_rectangle_fast(ctx, 0, 0, 1280, 1024, 0);

	clock_gettime(CLOCK_MONOTONIC, &ts_start);

	/* Do rectangle fill setup - this stalls the graphics pipeline */
	newport_fill_rectangle_setup(ctx);
	for (int i = 0; i < tcount; i++) {
		int x, y, w, h, c;

		x = random() % 1280;
		y = random() % 1024;
		w = tw;
		h = th;
		c = random() % 0xffffff;

		/* This doesn't stall the graphics pipeline */
		newport_fill_rectangle(ctx, x, y, w, h, c);
	}
	clock_gettime(CLOCK_MONOTONIC, &ts_end);
	ts = (ts_end.tv_sec * 1000000) + (ts_end.tv_nsec / 1000);
	ts = ts - ((ts_start.tv_sec * 1000000) + (ts_start.tv_nsec / 1000));
	fills_per_sec = (float) (((float) tcount * 1000.0) /
	    ((float) ts / 1000.0));
	pixels_per_sec = fills_per_sec * (float) th * (float) tw;

	printf("newport: %dx%d: %d fills in %llu milliseconds, "
	    "%.3f fills/sec, %.3f pixels/sec\n",
	    tw, th,
	    tcount,
	    ts / 1000,
	    fills_per_sec,
	    pixels_per_sec);
}

int
main(int argc, const char *argv[])
{
	struct gfx_ctx ctx;
	uint32_t arg1;

	if (! verify_newport()) {
		err(127, "Not a newport!\n");
	}
	printf("Hi! It's a newport!\n");

	gfx_ctx_init(&ctx);
	if (!newport_open(&ctx))
		exit(127);

	printf("DRAWMODE0: 0x%08x\n", rex3_read(&ctx, REX3_REG_DRAWMODE0));
	printf("DRAWMODE1: 0x%08x\n", rex3_read(&ctx, REX3_REG_DRAWMODE1));

	/* Set configuration to use */
	ctx.fb_mode = NewportBppModeRgb8; /* output is rgb8 */
	ctx.pixel_mode = NewportBppModeRgb24; /* input is rgb888 */
	ctx.cfreq = 70; /* 1024x768 60Hz */

	newport_setup_hw(&ctx);

	arg1 = 1;
	if (argc > 1)
		arg1 = strtoul(argv[1], NULL, 0);

	newport_fill_rectangle_fast(&ctx, 0, 0, 1280, 1024, 0);

	benchmark_rectangle(&ctx, 8, 8, arg1);
	benchmark_rectangle(&ctx, 16, 16, arg1);
	benchmark_rectangle(&ctx, 32, 32, arg1);
	benchmark_rectangle(&ctx, 64, 64, arg1);
	benchmark_rectangle(&ctx, 128, 128, arg1);

	newport_close(&ctx);

	exit(0);
}
