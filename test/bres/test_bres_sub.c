#include <stdio.h>
#include <stdbool.h>
#include "libbres/point.h"
#include "libbres/scanline.h"
#include "libbres_sub/bres_sub.h"

int main() {
    struct point2d p[3];
    struct scanline_list *slist;
    
    /* Third triangle from file */
    p[0].x = 742; p[0].y = 480;
    p[1].x = 756; p[1].y = 470;
    p[2].x = 754; p[2].y = 444;
    
    bres_sub_triangle(p, &slist);
    
    printf("BRES_SUB Triangle (8-bit subpixel): (742,480), (756,470), (754,444)\n");
    printf("Total scanlines: %d\n\n", slist->count);
    
    int total_pixels = 0;
    for (int i = 0; i < slist->count; i++) {
        int w = slist->list[i].x2 - slist->list[i].x1 + 1;
        total_pixels += w;
        printf("-- (%d -> %d, %d) width=%d\n", 
               slist->list[i].x1, slist->list[i].x2, slist->list[i].y, w);
    }
    printf("\nTotal pixels: %d\n", total_pixels);
    
    scanline_list_free(slist);
    return 0;
}
