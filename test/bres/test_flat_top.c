#include <stdio.h>
#include <stdbool.h>
#include "libbres/point.h"
#include "libbres/scanline.h"
#include "libbres/bres.h"

int main() {
    struct point2d p[3];
    struct scanline_list *slist;
    
    /* Flat-top test */
    p[0].x = 0; p[0].y = 0;
    p[1].x = 100; p[1].y = 0;
    p[2].x = 50; p[2].y = 10;
    
    bres_triangle(p, &slist);
    printf("Flat-top: %d scanlines\n", slist->count);
    for (int i = 0; i < slist->count; i++) {
        printf("  y=%d: (%d, %d)\n", slist->list[i].y, slist->list[i].x1, slist->list[i].x2);
    }
    scanline_list_free(slist);
    
    return 0;
}
