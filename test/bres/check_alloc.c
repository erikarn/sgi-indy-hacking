#include <stdio.h>
#include <stdbool.h>
#include "libbres/point.h"
#include "libbres/scanline.h"
#include "libbres/bres.h"

int main() {
    struct point2d p[3];
    
    p[0].x = 250; p[0].y = 50;
    p[1].x = 150; p[1].y = 300;
    p[2].x = 400; p[2].y = 550;
    
    // After sorting: a=(250,50), b=(150,300), c=(400,550)
    printf("Height for allocation: %d\n", p[2].y - p[0].y);
    printf("Flat-bottom height: %d (50 to 300)\n", 300 - 50);
    printf("Flat-top height: %d (550 to 300)\n", 550 - 300);
    printf("Total expected: %d\n", (300 - 50 + 1) + (550 - 300));
    
    return 0;
}
