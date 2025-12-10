#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <dev/wscons/wsconsio.h>
#include <sys/ioctl.h>
#include <err.h>

static bool
verify_newport(void)
{
	int fd, type, i;

	fd = open("/dev/ttyE0", O_RDONLY, 0);
	i = ioctl(fd, WSDISPLAYIO_GTYPE, &type);
	close(fd);

	return ((i == 0) && (type == WSDISPLAY_TYPE_NEWPORT));
}

int
main(int argc, const char *argv[])
{
	if (! verify_newport()) {
		err(127, "Not a newport!\n");
	}
	printf("Hi! It's a newport!\n");
	exit(0);
}
