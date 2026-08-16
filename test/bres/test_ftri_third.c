#include <stdio.h>
#include <stdbool.h>
#include "libbres/point.h"
#include "libbres/scanline.h"
#include "libftri/ftri.h"

int main() {
    struct point2d p[3];
    struct scanline_list *slist;
    
    /* Third triangle from file */
    p[0].x = 742; p[0].y = 480;
    p[1].x = 756; p[1].y = 470;
    p[2].x = 754; p[2].y = 444;
    
    ftri_triangle(p, &slist);
    
    printf("FTRI Triangle: (742,480), (756,470), (754,444)\n");
    printf("Total scanlines: %d\n\n", slist->count);
    
    for (int i = 0; i < slist->count; i++) {
        printf("-- (%d -> %d, %d)\n", 
               slist->list[i].x1, slist->list[i].x2, slist->list[i].y);
    }
    
    scanline_list_free(slist);
    return 0;
}
