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
