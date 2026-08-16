#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "libbres/point.h"
#include "libbres/scanline.h"
#include "libbres/bres.h"
#include "libftri/ftri.h"
#include "libbres_sub/bres_sub.h"

/* Parse a line like: triangle: (314,676), (344,618), (294,672) */
int parse_triangle_line(const char *line, int *x1, int *y1, int *x2, int *y2, 
                        int *x3, int *y3)
{
	const char *p = strstr(line, "triangle:");
	if (!p)
		return -1;
	
	p += 9; /* skip "triangle:" */
	
	if (sscanf(p, " (%d,%d), (%d,%d), (%d,%d)", x1, y1, x2, y2, x3, y3) == 6)
		return 0;
	
	return -1;
}

void test_triangle_with_all_methods(const char *name, int x1, int y1, 
                                     int x2, int y2, int x3, int y3)
{
	struct point2d p[3];
	struct scanline_list *slist_bres, *slist_ftri, *slist_sub;
	
	p[0].x = x1; p[0].y = y1;
	p[1].x = x2; p[1].y = y2;
	p[2].x = x3; p[2].y = y3;
	
	bres_triangle(p, &slist_bres);
	ftri_triangle(p, &slist_ftri);
	bres_sub_triangle(p, &slist_sub);
	
	int bres_pixels = 0, ftri_pixels = 0, sub_pixels = 0;
	for (int i = 0; i < slist_bres->count; i++)
		bres_pixels += (slist_bres->list[i].x2 - slist_bres->list[i].x1 + 1);
	for (int i = 0; i < slist_ftri->count; i++)
		ftri_pixels += (slist_ftri->list[i].x2 - slist_ftri->list[i].x1 + 1);
	for (int i = 0; i < slist_sub->count; i++)
		sub_pixels += (slist_sub->list[i].x2 - slist_sub->list[i].x1 + 1);
	
	printf("%-30s BRES=%4d  SUB=%4d  FTRI=%4d  ", 
	       name, bres_pixels, sub_pixels, ftri_pixels);
	
	if (ftri_pixels > 0) {
		double bres_err = 100.0 * abs(bres_pixels - ftri_pixels) / ftri_pixels;
		double sub_err = 100.0 * abs(sub_pixels - ftri_pixels) / ftri_pixels;
		printf("BRES err=%.1f%%  SUB err=%.1f%%\n", bres_err, sub_err);
	} else {
		printf("\n");
	}
	
	scanline_list_free(slist_bres);
	scanline_list_free(slist_ftri);
	scanline_list_free(slist_sub);
}

void test_triangle_file(const char *filename)
{
	FILE *fp;
	char line[512];
	int tri_num = 0;
	
	fp = fopen(filename, "r");
	if (!fp) {
		perror("Failed to open triangle file");
		return;
	}
	
	printf("\n=== Testing triangles from %s ===\n", filename);
	printf("%-30s %6s %6s %6s  %8s %8s\n", 
	       "Triangle", "BRES", "SUB", "FTRI", "BRES err", "SUB err");
	printf("%-30s %6s %6s %6s  %8s %8s\n", 
	       "--------", "----", "---", "----", "--------", "-------");
	
	while (fgets(line, sizeof(line), fp)) {
		int x1, y1, x2, y2, x3, y3;
		
		if (parse_triangle_line(line, &x1, &y1, &x2, &y2, &x3, &y3) == 0) {
			char name[128];
			snprintf(name, sizeof(name), "Triangle %d", tri_num + 1);
			test_triangle_with_all_methods(name, x1, y1, x2, y2, x3, y3);
			tri_num++;
		}
	}
	
	fclose(fp);
	printf("Tested %d triangles\n", tri_num);
}

int main(int argc, const char *argv[])
{
	const char *test_file = "test/bres/triangles/test_triangles.txt";
	
	if (argc > 1) {
		test_file = argv[1];
	}
	
	printf("Running Bresenham vs Sub-pixel vs Linear Interpolation comparison...\n");
	printf("Sub-pixel precision: %d bits (scale=%d)\n\n", 
	       SUBPIXEL_BITS, SUBPIXEL_SCALE);
	
	test_triangle_file(test_file);
	
	return 0;
}
