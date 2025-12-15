#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <sys/types.h>


/*
 * Experiments rendering a triangle from (x1,y1) -> (x2l, y2) / (x2r, y2)
 *
 * Ie, a shared starting point, with a flat bottomed triangle at the end.
 * Two bresenham iterators will occur, one for the left and one for the
 * right hand side.
 *
 * Inspired by https://mcejp.github.io/2020/11/06/bresenham.html
 *
 * TODO: do i have all the error terms correct?
 */
void
test_triangle_1(int x1, int y1, int x2l, int x2r, int y2)
{
	int xl_sign = (x2l < x1) ? -1 : 1;
	int xr_sign = (x2r < x1) ? -1 : 1;
	int E_left = -(y2 - y1) - (x2l - x1);
	int E_right = -(y2 - y1) - (x2r - x1);
	int X_left = x1;
	int X_right = x1;
	int yi;

	printf("%s:  (%d,%d) -> (%d,%d), (%d,%d) \n",
	    __func__, x1, y1, x2l, y2, x2r, y2);
	//printf("%s: xl_sign=%d, xr_sign=%d\n", __func__, xl_sign, xr_sign);

	for (yi = y1; yi < y2; yi++) {
		//printf("%s:  e_left=%d, e_right=%d\n", __func__, E_left, E_right);
		/* left hand iterator */
		while (E_left >= 0) {
			X_left += xl_sign;
			E_left -= 2* (y2 - y1);
		}

		/* right hand iterator */
		while (E_right >= 0) {
			X_right += xr_sign;
			E_right -= 2* (y2 - y1);
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
	test_triangle_1(40, 20, 5,  60,  30);
	test_triangle_1(40, 20, 5,  10,  30);
	test_triangle_1(40, 20, 50,  60,  30);
	test_triangle_1(40, 20, 30,  60,  30);
	exit(0);
}
