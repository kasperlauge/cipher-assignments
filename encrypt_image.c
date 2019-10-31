 /*
 * Image encryption tool.
 * 
 * Usage: encrypt_image [-e|-d] [-v|-a|-t] input_image output_image
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ppm.h"

/* Helper prototypes */
int modInverse(int a, int m);
void gmix_column(unsigned char *r);
void inv_gmix_column(unsigned char *r);
unsigned char sbox(unsigned char byte);
unsigned char inv_sbox(unsigned char byte);
/* Function prototypes. */
void vigenere(unsigned char *buf, int len);
void affine_enc(unsigned char *buf, int len);
void affine_dec(unsigned char *buf, int len);
void aes_enc(unsigned char *buf, int len);
void aes_dec(unsigned char *buf, int len);

/* Program arguments. */
FILE *input, *output;

/* Main program. */
int main(int argc, char *argv[]) {
	int encrypt = 0;
	pic image;

	/* Check number of arguments. */
	if (argc != 5) {
		fprintf(stderr,
				"\nUsage: encrypt_image [-e|-d] [-v|-a|-t] input_image output_image\n");
		exit(1);
	}
	/* Check mode. */
	if (argv[1][0] != '-' || strlen(argv[1]) != 2) {
		fprintf(stderr,
				"\nUse -e to encrypt and -d to decrypt..\n");
		exit(1);
	}
	encrypt = (argv[1][1] == 'e');
	/* Check algorithm. */
	if (argv[2][0] != '-' || strlen(argv[1]) != 2) {
		fprintf(stderr, "\nUnknown algorithm, use [-v|-a|-t].\n");
		exit(1);
	}
	/* Check files. */
	if ((input = fopen(argv[3], "rb")) == NULL) {
		fprintf(stderr, "\nN�o � poss�vel ler arquivo de entrada.\n");
		exit(1);
	}
	if ((output = fopen(argv[4], "wb")) == NULL) {
		fprintf(stderr, "\nN�o � poss�vel gravar em arquivo de sa�da.\n");
		exit(1);
	}

	if (ppm_read(input, &image) == 0) {
		fprintf(stderr, "\nN�o � poss�vel ler image de entrada.\n");
		exit(1);
	}

	/* Pass control to chosen algorithm. */
	switch (argv[2][1]) {
		case 'v':
			vigenere(image.pix, image.nx * image.ny * 3);
			break;
		case 'a':
			if (encrypt)
				affine_enc(image.pix, image.nx * image.ny * 3);
			else
				affine_dec(image.pix, image.nx * image.ny * 3);
			break;
		case 't':
                        if (encrypt)
                                aes_enc(image.pix, image.nx * image.ny * 3);
                        else
                                aes_dec(image.pix, image.nx * image.ny * 3);
                        break;
	}

	if (ppm_write(output, &image) == 0) {
		fprintf(stderr, "\nN�o � poss�vel gravar image de sa�da.\n");
		exit(1);
	}

	fclose(input);
	fclose(output);
}

/* Implements the Vigenere cipher. */
void vigenere(unsigned char *buf, int len) {
	int i, length;
	unsigned int byte;
	unsigned char *key;

	printf("Key size: ");
	if (scanf("%d", &length) != 1) {
		fprintf(stderr, "\nInvalid key size.\n");
		exit(1);
	}
	key = (char *)malloc((length + 1) * (sizeof(char)));
	getchar();

	printf("Encryption key: ");
	for (i = 0; i < length; i++) {
		if (scanf("%u", &byte) != 1) {
			fprintf(stderr, "\nError reading encryption key.\n");
			exit(1);
		}
		key[i] = byte % 256;
	}
	key[length] = '\0';

	for (i = 0; i < len; i++) {
		buf[i] = buf[i] ^ key[i % length];
	}
	free(key);
}

void affine_enc(unsigned char *buf, int len) {
	unsigned int a, b, hcf, i, j, y, x;
	unsigned int m = 256;
	printf("Integer a (between 0 and 255): ");
	if (scanf("%u", &a) != 1) {
		fprintf(stderr, "\nInvalid key size.\n");
		exit(1);
	}
	printf("Integer b (between 0 and 255): ");
	if (scanf("%u", &b) != 1) {
		fprintf(stderr, "\nInvalid key size.\n");
		exit(1);
	}

	if (a > 255 || b > 255) {
		fprintf(stderr, "\nInvalid integer size.\n");
		exit(1);
	}

	// Check if a and m are co-prime
	for(i = 1; i <= m; i++) {
		if (m % i == 0 && a % i == 0) {
	   	hcf = i;
	  }
	}

	if (hcf != 1) {
		fprintf(stderr, "\na and %d are not co-prime\n", m);
		exit(1);
	}

	for (j = 0; j < len; j++) {
		x = buf[j];
		y = (a*x + b) % m;
		buf[j] = (unsigned char)y;
	}
}

void affine_dec(unsigned char *buf, int len) {
	unsigned int a, b, hcf, i, j, y, d, inv_a;
	unsigned int m = 256;
	printf("Integer a (between 0 and 255): ");
	if (scanf("%u", &a) != 1) {
		fprintf(stderr, "\nInvalid key size.\n");
		exit(1);
	}
	printf("Integer b (between 0 and 255): ");
	if (scanf("%u", &b) != 1) {
		fprintf(stderr, "\nInvalid key size.\n");
		exit(1);
	}

	if (a > 255 || b > 255) {
		fprintf(stderr, "\nInvalid integer size.\n");
		exit(1);
	}

	// Check if a and m are co-prime
	for(i = 1; i <= m; i++) {
		if (m % i == 0 && a % i == 0) {
	   	hcf = i;
	  }
	}

	if (hcf != 1) {
		fprintf(stderr, "\na and %d are not co-prime\n", m);
		exit(1);
	}

	for (j = 0; j < len; j++) {
		y = buf[j];
		inv_a = modInverse(a, m);
		d = (inv_a*(y-b)) % m;
		buf[j] = (unsigned char)d;
	}
}

void aes_enc(unsigned char *buf, int len) {
	int i, j, k, l, m, n, r;
	int length = 4;
	unsigned int rounds = 9;
	unsigned int block_size = 16;
	unsigned char *block;
	unsigned int bytes;
	unsigned char byte, swap;
	unsigned int key_size = (length*sizeof(uint32_t) + 1) * (sizeof(unsigned char));
	unsigned int column_size = (length + 1) * (sizeof(unsigned char));
	unsigned char *key_column;
	unsigned char *block_row;
	unsigned char *block_column;
	unsigned char *key;
	unsigned char *prev_key;
	unsigned char *round_keys[rounds];
	unsigned char rc, prev;
	unsigned char round_constants[rounds][sizeof(uint32_t)];

	key = (unsigned char *)malloc(key_size);
	prev_key = (unsigned char *)malloc(key_size);
	key_column = (unsigned char *)malloc(column_size);
	block_row = (unsigned char *)malloc(column_size);
	block_column = (unsigned char *)malloc(column_size);
	block = (unsigned char *)malloc(key_size);

	for (i = 0; i < rounds; i++) {
		round_keys[i] = (unsigned char *)malloc(key_size);
	}
	// getchar();

	printf("Encryption key: ");
	for (j = 0; j < length; j++) {
		if (scanf("%x", &bytes) != 1) {
			fprintf(stderr, "\nError reading encryption key.\n");
			exit(1);
		}
		for (k = 0; k < length; k++) {
			byte = bytes >> (-1*(k - (length - 1)) * 8) & 0xFF;
			key[j * length + k] = byte;
		}
	}
	key[key_size] = '\0';

	// Actual AES
	// Generate round keys

	// Define round constants: https://en.wikipedia.org/wiki/AES_key_schedule
	for (n = 0; n < rounds; n++) {
		if (n == 0) {
			rc = 1;
		} else if (n > 0 && prev < 0x80) {
			rc = 2 * prev;
		} else {
			rc = (2 * prev) ^ 0x1B;
		}
		round_constants[n][0] = rc;
		round_constants[n][1] = 0x00;
		round_constants[n][2] = 0x00;
		round_constants[n][3] = 0x00;
		prev = rc;
	}

	// Generate round keys
	for (i = 0; i < rounds; i++) {
		if (i == 0) {
			// Fill prev_key with initial key
			for (m = 0; m < key_size; m++) {
				prev_key[m] = key[m];
			}
		} else {
			// Otherwise fill prev_key with previous round key
			for (m = 0; m < key_size; m++) {
				prev_key[m] = round_keys[i - 1][m];
			}
		}

		prev_key[key_size] = '\0';

		// Take last column of prev_key
		for (m = 0; m < length; m++) {
			key_column[m] = prev_key[length * m + length - 1];
		}

		key_column[length] = '\0';

		// Swap last block with first block
		swap = key_column[length - 1];
		key_column[length - 1] = key_column[0];
		key_column[0] = swap;

		// Run each byte through sbox
		for (m = 0; m < length; m++) {
			key_column[m] = sbox(key_column[m]);
		}

		// XOR with round constant
		for (m = 0; m < length; m++) {
			key_column[m] = key_column[m] ^ round_constants[i][m];
		}

		// XOR with the first column of previous key
			for (m = 0; m < length; m++) {
			key_column[m] = key_column[m] ^ prev_key[length * m];
		}

		// Fill the round key first column
		for (m = 0; m < length; m++) {
			round_keys[i][m * length] = key_column[m];
		}

		// Fill the rest of the columns of the round key
		for (m = 1; m < length; m++) {
			for (r = 0; r < length; r++) {
				round_keys[i][length * r + m] = prev_key[length * r * m] ^ round_keys[i][length * r + (m - 1)];
			}
		}

		round_keys[i][key_size] = '\0';


		// For each block
		for (j = 0; j < len; j+=block_size) {
			if ((len - j) < block_size) {
				// We reached last block

			} else {
				// Fill block normally
				for (m = 0; m < block_size; m++) {
					block[m] = buf[j + m];
				}
			}
			block[block_size] = '\0';

			// Initial round - just XOR input with initial key
			for (m = 0; m < block_size; m++) {
				block[m] = block[m] ^ key[m];
			}

			// Intermediate rounds
			for (n = 0; n < rounds; n++) {

				// Apply confusion
				for (m = 0; m < block_size; m++) {
					block[m] = sbox(block[m]);
				}

				// Apply diffusion
				for (k = 1; k < length; k++) {
					for (l = 0; l < length; l++) {
						block_row[l] = block[k * length + l];
					}
					for (l = 0; l < length; l++) {
						block[l + length*3] = block_row[(l + k) % (length - 1)];
					}
				}

				// Mix columns
				// Only if it is not the final round
				if (n < (rounds - 1)) {
					for (k = 0; k < length; k++) {
						for (l = 0; l < length; l++) {
							block_column[l] = block[l * length + k];
						}
						gmix_column(block_column);
						for (l = 0; l < length; l++) {
							block[l * length + k] = block_column[l];
						}
					}
				}

				// Apply key secrecy
				for (k = 0; k < block_size; k++) {
					block[k] = block[k] ^ round_keys[n][k];
				}

			}

			// Scoop block into buf again
			for (m = 0; m < block_size; m++) {
					buf[j + m] = block[m];
			}

		}
	}

	for (l = 0; l < rounds - 1; l++) {
		free(round_keys[l]);
	}
	free(key_column);
	free(block_row);
	free(block_column);
	free(key);
	free(prev_key);
	free(block);
}

void aes_dec(unsigned char *buf, int len) {
	int i, j, k, l, m, n, r;
	int length = 4;
	unsigned int rounds = 9;
	unsigned int block_size = 16;
	unsigned char *block;
	unsigned int bytes;
	unsigned char byte, swap;
	unsigned int key_size = (length*sizeof(uint32_t) + 1) * (sizeof(unsigned char));
	unsigned int column_size = (length + 1) * (sizeof(unsigned char));
	unsigned char *key_column;
	unsigned char *block_row;
	unsigned char *block_column;
	unsigned char *key;
	unsigned char *prev_key;
	unsigned char *round_keys[rounds];
	unsigned char rc, prev;
	unsigned char round_constants[rounds][sizeof(uint32_t)];

	key = (unsigned char *)malloc(key_size);
	prev_key = (unsigned char *)malloc(key_size);
	key_column = (unsigned char *)malloc(column_size);
	block_row = (unsigned char *)malloc(column_size);
	block_column = (unsigned char *)malloc(column_size);
	block = (unsigned char *)malloc(key_size);

	for (i = 0; i < rounds; i++) {
		round_keys[i] = (unsigned char *)malloc(key_size);
	}
	// getchar();

	printf("Encryption key: ");
	for (j = 0; j < length; j++) {
		if (scanf("%x", &bytes) != 1) {
			fprintf(stderr, "\nError reading encryption key.\n");
			exit(1);
		}
		for (k = 0; k < length; k++) {
			byte = bytes >> (-1*(k - (length - 1)) * 8) & 0xFF;
			key[j * length + k] = byte;
		}
	}
	key[key_size] = '\0';

	// Actual AES
	// Generate round keys

	// Define round constants: https://en.wikipedia.org/wiki/AES_key_schedule
	for (n = 0; n < rounds; n++) {
		if (n == 0) {
			rc = 1;
		} else if (n > 0 && prev < 0x80) {
			rc = 2 * prev;
		} else {
			rc = (2 * prev) ^ 0x1B;
		}
		round_constants[n][0] = rc;
		round_constants[n][1] = 0x00;
		round_constants[n][2] = 0x00;
		round_constants[n][3] = 0x00;
		prev = rc;
	}

	// Generate round keys
	for (i = 0; i < rounds; i++) {
		if (i == 0) {
			// Fill prev_key with initial key
			for (m = 0; m < key_size; m++) {
				prev_key[m] = key[m];
			}
		} else {
			// Otherwise fill prev_key with previous round key
			for (m = 0; m < key_size; m++) {
				prev_key[m] = round_keys[i - 1][m];
			}
		}

		prev_key[key_size] = '\0';

		// Take last column of prev_key
		for (m = 0; m < length; m++) {
			key_column[m] = prev_key[length * m + length - 1];
		}

		key_column[length] = '\0';

		// Swap last block with first block
		swap = key_column[length - 1];
		key_column[length - 1] = key_column[0];
		key_column[0] = swap;

		// Run each byte through sbox
		for (m = 0; m < length; m++) {
			key_column[m] = sbox(key_column[m]);
		}

		// XOR with round constant
		for (m = 0; m < length; m++) {
			key_column[m] = key_column[m] ^ round_constants[i][m];
		}

		// XOR with the first column of previous key
			for (m = 0; m < length; m++) {
			key_column[m] = key_column[m] ^ prev_key[length * m];
		}

		// Fill the round key first column
		for (m = 0; m < length; m++) {
			round_keys[i][m * length] = key_column[m];
		}

		// Fill the rest of the columns of the round key
		for (m = 1; m < length; m++) {
			for (r = 0; r < length; r++) {
				round_keys[i][length * r + m] = prev_key[length * r * m] ^ round_keys[i][length * r + (m - 1)];
			}
		}

		round_keys[i][key_size] = '\0';


		// For each block
		for (j = 0; j < len; j+=block_size) {
			if ((len - j) < block_size) {
				// We reached last block

			} else {
				// Fill block normally
				for (m = 0; m < block_size; m++) {
					block[m] = buf[j + m];
				}
			}
			block[block_size] = '\0';

			// Intermediate rounds
			for (n = 0; n < rounds; n++) {

				// Apply key secrecy
				for (k = 0; k < block_size; k++) {
					block[k] = block[k] ^ round_keys[n][k];
				}

				// Mix columns
				// Only if it is not the first round
				if (n > 0) {
					for (k = 0; k < length; k++) {
						for (l = 0; l < length; l++) {
							block_column[l] = block[l * length + k];
						}
						inv_gmix_column(block_column);
						for (l = 0; l < length; l++) {
							block[l * length + k] = block_column[l];
						}
					}
				}

				// Apply diffusion
				for (k = 1; k < length; k++) {
					for (l = 0; l < length; l++) {
						block_row[l] = block[k * length + l];
					}
					for (l = 0; l < length; l++) {
						block[l + length*3] = block_row[(l + k) % (length - 1)];
					}
				}

				// Apply confusion
				for (m = 0; m < block_size; m++) {
					block[m] = sbox(block[m]);
				}
			}

			// Last round - just XOR input with initial key
			for (m = 0; m < block_size; m++) {
				block[m] = block[m] ^ key[m];
			}

			// Scoop block into buf again
			for (m = 0; m < block_size; m++) {
					buf[j + m] = block[m];
			}

		}
	}

	for (l = 0; l < rounds - 1; l++) {
		free(round_keys[l]);
	}
	free(key_column);
	free(block_row);
	free(block_column);
	free(key);
	free(prev_key);
	free(block);
}


// From https://www.geeksforgeeks.org/multiplicative-inverse-under-modulo-m/
int modInverse(int a, int m) { 
    a = a % m; 
    for (int x = 1; x < m; x++) 
       if ((a * x) % m == 1) 
          return x; 
}

// From https://en.wikipedia.org/wiki/Rijndael_S-box
unsigned char sbox(unsigned char byte) {
	return byte ^ (byte << 1) ^ (byte << 2) ^ (byte << 3) ^ (byte << 4) ^ 0x63;
}

// From https://en.wikipedia.org/wiki/Rijndael_S-box
unsigned char inv_sbox(unsigned char byte) {
	return (byte << 1) ^ (byte << 3) ^ (byte << 6) ^ 0x5;
}

// From https://en.wikipedia.org/wiki/Rijndael_MixColumns
void gmix_column(unsigned char *r) {
    unsigned char a[4];
    unsigned char b[4];
    unsigned char c;
    unsigned char h;
    /* The array 'a' is simply a copy of the input array 'r'
     * The array 'b' is each element of the array 'a' multiplied by 2
     * in Rijndael's Galois field
     * a[n] ^ b[n] is element n multiplied by 3 in Rijndael's Galois field */ 
    for (c = 0; c < 4; c++) {
        a[c] = r[c];
        /* h is 0xff if the high bit of r[c] is set, 0 otherwise */
        h = (unsigned char)((signed char)r[c] >> 7); /* arithmetic right shift, thus shifting in either zeros or ones */
        b[c] = r[c] << 1; /* implicitly removes high bit because b[c] is an 8-bit char, so we xor by 0x1b and not 0x11b in the next line */
        b[c] ^= 0x1B & h; /* Rijndael's Galois field */
    }
    r[0] = b[0] ^ a[3] ^ a[2] ^ b[1] ^ a[1]; /* 2 * a0 + a3 + a2 + 3 * a1 */
    r[1] = b[1] ^ a[0] ^ a[3] ^ b[2] ^ a[2]; /* 2 * a1 + a0 + a3 + 3 * a2 */
    r[2] = b[2] ^ a[1] ^ a[0] ^ b[3] ^ a[3]; /* 2 * a2 + a1 + a0 + 3 * a3 */
    r[3] = b[3] ^ a[2] ^ a[1] ^ b[0] ^ a[0]; /* 2 * a3 + a2 + a1 + 3 * a0 */
}

void inv_gmix_column(unsigned char *r) {
	gmix_column(r);
	gmix_column(r);
	gmix_column(r);
}