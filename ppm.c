#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>

#include "ppm.h"

/*
 * ppm: subroutines for reading PPM picture files
 *
 * Adapted from the work of Paul Heckbert and Michael Garland.
 *
 */

static char *ppm_get_token(FILE *fp, char *tok, int len) {
	char *t;
	int c;

	for (;;) { /* skip over whitespace and comments */
		while (isspace(c = getc(fp)));
		if (c != '#')
			break;
		do
			c = getc(fp);
		while (c != '\n' && c != EOF); /* gobble comment */
		if (c == EOF)
			break;
	}

	t = tok;
	if (c != EOF) {
		do {
			*t++ = c;
			c = getc(fp);
		} while (!isspace(c) && c != '#' && c != EOF && t - tok < len - 1);
		if (c == '#')
			ungetc(c, fp); /* put '#' back for next time */
	}
	*t = 0;
	return tok;
}

/* ppm_get_size: get size in pixels of PPM picture file */
static int ppm_get_size(char *file, int *nx, int *ny) {
	char tok[20];
	FILE *fp;

	if ((fp = fopen(file, "r")) == NULL) {
		fprintf(stderr, "can't read PPM file %s\n", file);
		return 0;
	}
	if (strcmp(ppm_get_token(fp, tok, sizeof tok), "P6")) {
		fprintf(stderr, "%s is not a valid binary PPM file, bad magic#\n",
				file);
		fclose(fp);
		return 0;
	}
	if (sscanf(ppm_get_token(fp, tok, sizeof tok), "%d", nx) != 1 ||
			sscanf(ppm_get_token(fp, tok, sizeof tok), "%d", ny) != 1) {
		fprintf(stderr, "%s is not a valid PPM file: bad size\n", file);
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return 1;
}

int ppm_read(FILE *fp, pic *p) {
	char tok[20];
	int nx, ny, pvmax;

	/* read PPM header */
	if (strcmp(ppm_get_token(fp, tok, sizeof tok), "P6")) {
		fprintf(stderr, "error: not a valid binary PPM file, bad magic#\n");
		return 0;
	}
	if (sscanf(ppm_get_token(fp, tok, sizeof tok), "%d", &nx) != 1 ||
			sscanf(ppm_get_token(fp, tok, sizeof tok), "%d", &ny) != 1 ||
			sscanf(ppm_get_token(fp, tok, sizeof tok), "%d", &pvmax) != 1) {
		fprintf(stderr, "error: not a valid PPM file: bad size\n");
		return 0;
	}

	if (pvmax != 255) {
		fprintf(stderr, "error: does not have 8-bit components: pvmax=%d\n", pvmax);
		return 0;
	}

	p->nx = nx;
	p->ny = ny;
	p->pix = (unsigned char *)malloc(nx * ny * 3 * sizeof(unsigned char));

	if (fread(p->pix, p->nx * 3, p->ny, fp) != p->ny) { /* read pixels */
		fprintf(stderr, "error: premature EOF on file %s\n");
		free(p->pix);
		return 0;
	}

	return 1;
}

int ppm_write(FILE *fp, pic *pic) {
	/* Always write a raw PPM file */
	fprintf(fp, "P6 %d %d 255\n", pic->nx, pic->ny);

	if (fwrite(pic->pix, pic->nx * 3, pic->ny, fp) != pic->ny) {
		fprintf(stderr, "error: error writing file\n");
		return 0;
	}

	free(pic->pix);

	return 1;
}
