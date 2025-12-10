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
	ctx->fb_mode = NewportBppModeCi8;
	ctx->pixel_mode = NewportBppModeCi8;
	ctx->pixel_mode = 70; /* 1024x768 60Hz */
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

int
main(int argc, const char *argv[])
{
	struct gfx_ctx ctx;

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
	ctx.fb_mode = NewportBppModeCi8;
	ctx.pixel_mode = NewportBppModeCi8;
	ctx.pixel_mode = 70; /* 1024x768 60Hz */

	newport_setup_hw(&ctx);

	sleep(1);

	newport_close(&ctx);

	exit(0);
}
