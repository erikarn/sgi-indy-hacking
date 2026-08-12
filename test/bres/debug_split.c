#include <stdio.h>
#include <stdbool.h>
#define PRINT_TRIANGLE_SETUP
#define DEBUG_TRIANGLE
#include "libbres/point.h"
#include "libbres/scanline.h"
#include "libbres/bres.h"

int main() {
    struct point2d p[3];
    struct scanline_list *slist;
    
    p[0].x = 250; p[0].y = 50;
    p[1].x = 150; p[1].y = 300;
    p[2].x = 400; p[2].y = 550;
    
    bres_triangle(p, &slist);
    
    scanline_list_free(slist);
    return 0;
}
