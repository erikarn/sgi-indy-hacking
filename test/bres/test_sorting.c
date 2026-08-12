#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>

#include "libbres/point.h"
#include "libbres/scanline.h"
#include "libbres/bres.h"

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT(cond, msg) do { \
	tests_run++; \
	if (cond) { \
		tests_passed++; \
	} else { \
		tests_failed++; \
		printf("FAIL: %s\n", msg); \
	} \
} while(0)

/*
 * Test that vertices are sorted by Y coordinate correctly.
 * After sorting: plist[a].y <= plist[b].y <= plist[c].y
 */
void
test_vertex_sorting(void)
{
	struct point2d p[3];
	struct scanline_list *slist;

	printf("\n=== Testing vertex sorting ===\n");

	/* Test 1: Already sorted (0, 10, 20) */
	p[0].x = 0; p[0].y = 0;
	p[1].x = 10; p[1].y = 10;
	p[2].x = 20; p[2].y = 20;
	bres_triangle(p, &slist);
	ASSERT(slist != NULL, "vertices already sorted produces scanlist");
	scanline_list_free(slist);

	/* Test 2: Reverse order (20, 10, 0) */
	p[0].x = 20; p[0].y = 20;
	p[1].x = 10; p[1].y = 10;
	p[2].x = 0; p[2].y = 0;
	bres_triangle(p, &slist);
	ASSERT(slist != NULL, "reverse sorted vertices produces scanlist");
	scanline_list_free(slist);

	/* Test 3: Middle is smallest (10, 0, 20) */
	p[0].x = 10; p[0].y = 10;
	p[1].x = 0; p[1].y = 0;
	p[2].x = 20; p[2].y = 20;
	bres_triangle(p, &slist);
	ASSERT(slist != NULL, "middle-smallest vertices produces scanlist");
	scanline_list_free(slist);

	/* Test 4: All same Y (flat line - degenerate) */
	p[0].x = 0; p[0].y = 10;
	p[1].x = 10; p[1].y = 10;
	p[2].x = 20; p[2].y = 10;
	bres_triangle(p, &slist);
	/* This should handle gracefully - either empty or single span */
	scanline_list_free(slist);
}

/*
 * Test flat-bottom triangles render correctly.
 * Known good output for verification.
 */
void
test_flat_bottom_triangle(void)
{
	struct point2d p[3];
	struct scanline_list *slist;

	printf("\n=== Testing flat-bottom triangle ===\n");

	/* Simple flat-bottom: apex at top, base at bottom */
	p[0].x = 50; p[0].y = 0;   /* apex */
	p[1].x = 0; p[1].y = 10;   /* left base */
	p[2].x = 100; p[2].y = 10; /* right base */

	bres_triangle(p, &slist);

	ASSERT(slist != NULL, "flat-bottom triangle produces scanlist");
	ASSERT(slist->count == 11, "flat-bottom has 11 scanlines (y=0..10)");

	if (slist && slist->count > 0) {
		/* First scanline should be near x=50 */
		ASSERT(slist->list[0].y == 0, "first scanline at y=0");

		/* Last scanline should span wide */
		int last = slist->count - 1;
		ASSERT(slist->list[last].y == 10, "last scanline at y=10");
	}

	scanline_list_free(slist);
}

/*
 * Test flat-top triangles render correctly.
 */
void
test_flat_top_triangle(void)
{
	struct point2d p[3];
	struct scanline_list *slist;

	printf("\n=== Testing flat-top triangle ===\n");

	/* Simple flat-top: base at top, apex at bottom */
	p[0].x = 0; p[0].y = 0;    /* left base */
	p[1].x = 100; p[1].y = 0;  /* right base */
	p[2].x = 50; p[2].y = 10;  /* apex */

	bres_triangle(p, &slist);

	ASSERT(slist != NULL, "flat-top triangle produces scanlist");
	ASSERT(slist->count == 11, "flat-top has 11 scanlines (y=10..0)");

	if (slist && slist->count > 0) {
		/* First scanline should be at y=10 (apex) */
		ASSERT(slist->list[0].y == 10, "first scanline at y=10");

		/* Last scanline should be at y=0 */
		int last = slist->count - 1;
		ASSERT(slist->list[last].y == 0, "last scanline at y=0");
	}

	scanline_list_free(slist);
}

/*
 * Test arbitrary triangle splits into two parts correctly.
 */
void
test_arbitrary_triangle(void)
{
	struct point2d p[3];
	struct scanline_list *slist;

	printf("\n=== Testing arbitrary triangle ===\n");

	/* Triangle with no flat edges */
	p[0].x = 10; p[0].y = 10;
	p[1].x = 90; p[1].y = 50;
	p[2].x = 50; p[2].y = 90;

	bres_triangle(p, &slist);

	ASSERT(slist != NULL, "arbitrary triangle produces scanlist");
	ASSERT(slist->count > 0, "arbitrary triangle has scanlines");

	/* Verify each span has valid coordinates */
	for (int i = 0; i < slist->count; i++) {
		ASSERT(slist->list[i].x1 <= slist->list[i].x2,
		    "scanline has x1 <= x2");
	}

	scanline_list_free(slist);
}

/*
 * Test midpoint calculation precision.
 * The formula: mp.x = a.x + ((b.y-a.y)/(c.y-a.y))*(c.x-a.x)
 * With fixed-point: should avoid integer division before multiplication
 */
void
test_midpoint_precision(void)
{
	struct point2d p[3];
	struct scanline_list *slist;

	printf("\n=== Testing midpoint precision ===\n");

	/* Triangle where midpoint calculation matters */
	/* a=(0,0), b=(50,100), c=(100,200) */
	/* Expected midpoint at b.y=100: mp.x should be ~50 */
	p[0].x = 0; p[0].y = 0;
	p[1].x = 50; p[1].y = 100;
	p[2].x = 100; p[2].y = 200;

	bres_triangle(p, &slist);

	ASSERT(slist != NULL, "midpoint test produces scanlist");

	/* Find the scanline at y=100 (the split point) */
	bool found_split = false;
	for (int i = 0; i < slist->count; i++) {
		if (slist->list[i].y == 100) {
			found_split = true;
			/* At the split, x1 should be exactly 50 (vertex b) */
			ASSERT(slist->list[i].x1 == 50,
			    "midpoint x1 matches vertex b.x at split");
			break;
		}
	}
	ASSERT(found_split, "found scanline at split point y=100");

	scanline_list_free(slist);
}

/*
 * Test that scanline X coordinates are valid (x1 <= x2).
 */
void
test_scanline_validity(void)
{
	struct point2d p[3];
	struct scanline_list *slist;

	printf("\n=== Testing scanline validity ===\n");

	/* Various triangle orientations */
	int triangles[][6] = {
		{10, 10, 90, 50, 50, 90},   /* normal */
		{90, 10, 10, 50, 50, 90},   /* mirrored */
		{50, 90, 10, 10, 90, 50},   /* inverted */
		{0, 0, 100, 50, 50, 100},   /* large */
	};

	for (int t = 0; t < 4; t++) {
		p[0].x = triangles[t][0]; p[0].y = triangles[t][1];
		p[1].x = triangles[t][2]; p[1].y = triangles[t][3];
		p[2].x = triangles[t][4]; p[2].y = triangles[t][5];

		bres_triangle(p, &slist);

		if (slist) {
			for (int i = 0; i < slist->count; i++) {
				char msg[128];
				snprintf(msg, sizeof(msg),
				    "triangle %d scanline %d: x1 <= x2", t, i);
				ASSERT(slist->list[i].x1 <= slist->list[i].x2, msg);
			}
			scanline_list_free(slist);
		}
	}
}

int
main(int argc, const char *argv[])
{
	printf("Running libbres validation tests...\n");

	test_vertex_sorting();
	test_flat_bottom_triangle();
	test_flat_top_triangle();
	test_arbitrary_triangle();
	test_midpoint_precision();
	test_scanline_validity();

	printf("\n=== Test Results ===\n");
	printf("Run: %d, Passed: %d, Failed: %d\n",
	    tests_run, tests_passed, tests_failed);

	return tests_failed > 0 ? 1 : 0;
}
