/*
 * ppm: subroutines for reading PPM picture files
 *
 * Adapted from the work of Paul Heckbert and Michael Garland.
 *
 */

typedef struct {
	int nx, ny;
	unsigned char *pix;
} pic;
