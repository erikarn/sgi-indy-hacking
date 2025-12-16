#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

#include "scanline.h"

struct scanline_list *
scanline_list_alloc(int count)
{
	struct scanline_list *l;

	l = calloc(1, sizeof(*l));
	if (l == NULL)
		return NULL;

	l->count = count;
	l->cur = 0;
	l->list = calloc(count, sizeof(struct scanline_2d));
	if (l->list == NULL) {
		free(l);
		return NULL;
	}
	return l;
}

void
scanline_list_free(struct scanline_list *l)
{
	if (l == NULL)
		return;
	free(l->list);
	free(l);
}

bool
scanline_list_push(struct scanline_list *l, int x1, int x2, int y)
{
	if (l->cur >= l->count)
		return false;

	l->list[l->cur].x1 = x1;
	l->list[l->cur].x2 = x2;
	l->list[l->cur].y = y;
	l->cur++;
	return true;
}

void
scanline_list_print(const struct scanline_list *l, const char *pfx)
{
	int i;

	for (i = 0; i < l->cur; i++) {
		printf("%s(%d,%d) -> (%d,%d)\n", pfx,
		    l->list[l->cur].x1,
		    l->list[l->cur].y,
		    l->list[l->cur].x2,
		    l->list[l->cur].y);
	}
}
