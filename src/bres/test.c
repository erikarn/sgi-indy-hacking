#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <sys/types.h>


/*
 * Experiments rendering a flat top or bottom triangle
 * from (x1,y1) -> (x2l, y2) / (x2r, y2)
 *
 * This attempts to handle all the possible configurations:
 *
 * flat bottom (y1 < y2)
 * + xl < x < xr
 * + xl < xr < x
 * + x < xl < xr
 *
 * flat top (y1 > y2)
 * + xl < x < xr
 * + xl < xr < x
 * + x < xl < xr
 *
 * I haven't yet tested the degenerate cases (y1 == y2, x1 == x,
 * x2 == x, x1 == x == x2).
 *
 * Inspired by https://mcejp.github.io/2020/11/06/bresenham.html
 *
 * TODO: do i have all the error terms correct?
 */
void
test_triangle_1(int x1, int y1, int x2l, int x2r, int y2)
{
	const int xl_sign = (x2l < x1) ? -1 : 1;
	const int xr_sign = (x2r < x1) ? -1 : 1;
	int X_left = x1;
	int X_right = x1;
	int yi;
	const int y_sign = (y2 < y1) ? -1 : 1;
	const int yi1 = (y_sign == 1) ? y1 : y2;
	const int yi2 = (y_sign == 1) ? y2 : y1;
	const int yd = yi2 - yi1;
	int yc = (yi2 - yi1);
	int E_left = -(yi2 - yi1) - (x2l - x1);
	int E_right = -(yi2 - yi1) - (x2r - x1);

	printf("%s:  (%d,%d) -> (%d,%d), (%d,%d) \n",
	    __func__, x1, y1, x2l, y2, x2r, y2);
	//printf("%s: xl_sign=%d, xr_sign=%d\n", __func__, xl_sign, xr_sign);

	/*
	 * Handle y1 == y2; but ideally the caller would not bother and
	 * just render a single span.
	 */
	if (y1 == y2) {
		printf("%s:  (%d, %d),%d\n", __func__, X_left, X_right, y1);
		return;
	}

	for (yi = yi1; yc >= 0; yc--, yi += y_sign) {
		//printf("%s:  e_left=%d, e_right=%d\n", __func__, E_left, E_right);
		/* left hand iterator */
		while (E_left >= 0) {
			X_left += xl_sign;
			E_left -= 2* (yi2 - yi1);
		}

		/* right hand iterator */
		while (E_right >= 0) {
			X_right += xr_sign;
			E_right -= 2* (yi2 - yi1);
		}

		printf("%s:  (%d, %d),%d\n", __func__, X_left, X_right, yi);

		E_left -= 2 * -xl_sign * (x2l - x1);
		E_right -= 2 * -xr_sign * (x2r - x1);
	}
}

int
main(int argc, const char *argv[])
{
	             /* x1, y1, x2l, x2r,  y2 */

	/* y2 > y1 */
	test_triangle_1(40, 20, 5,  60,  30);
	test_triangle_1(40, 20, 5,  10,  30);
	test_triangle_1(40, 20, 50,  60,  30);
	test_triangle_1(40, 20, 30,  60,  30);
	test_triangle_1(40, 20, 40,  60,  30);
	test_triangle_1(40, 20, 30,  40,  30);

	/* y1 > y2 */
	test_triangle_1(40, 30, 5,  60,  20);
	test_triangle_1(40, 30, 5,  10,  20);
	test_triangle_1(40, 30, 50,  60,  20);
	test_triangle_1(40, 30, 30,  60,  20);
	test_triangle_1(40, 30, 40,  60,  20);
	test_triangle_1(40, 30, 30,  40,  20);

	/* y1 == y2 */
	test_triangle_1(40, 30, 5,  60,  30);
	test_triangle_1(40, 30, 5,  10,  30);
	test_triangle_1(40, 30, 50,  60,  30);
	test_triangle_1(40, 30, 30,  60,  30);
	test_triangle_1(40, 30, 40,  60,  30);
	test_triangle_1(40, 30, 30,  40,  30);

	exit(0);
}
