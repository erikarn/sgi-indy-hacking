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

/*
 * Note: I'm mmap()'ing the rex3 registers at 0x0, not 0xf0000.
 */

void
rex3_write(struct gfx_ctx *ctx, uint32_t rexreg, uint32_t val)
{
	volatile uint32_t *reg;

	reg = (uint32_t)(((char *) ctx->addr) + rexreg);
	*reg = val;
}

uint32_t
rex3_read(struct gfx_ctx *ctx, uint32_t rexreg)
{
	volatile uint32_t *reg;

	reg = (uint32_t)(((char *) ctx->addr) + rexreg);
	val = *reg;
}

void
rex3_write_go(struct gfx_ctx *ctx, uint32_t rexreg, uint32_t val)
{
	volatile uint32_t *reg;

	rex3_write(ctx, rexreg + REX3_REG_GO, val);
}
