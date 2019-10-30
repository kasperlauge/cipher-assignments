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
	unsigned int bytes;
	unsigned char byte, swap;
	unsigned int key_size = (length*sizeof(uint32_t) + 1) * (sizeof(unsigned char));
	unsigned int column_size = (length + 1) * (sizeof(unsigned char));
	unsigned char *key_column;
	unsigned char *key;
	unsigned char *prev_key;
	unsigned char *round_keys[rounds - 1];
	unsigned char rc, prev;
	unsigned char round_constants[rounds][sizeof(uint32_t)];

	key = (unsigned char *)malloc(key_size);
	prev_key = (unsigned char *)malloc(key_size);
	key_column = (unsigned char *)malloc(column_size);

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
		printf("\nRound %d:\n", i);
		// for (m = 0; m < key_size - 1; m++) {
		// 	printf("%x", prev_key[m]);
		// }
		printf("prev_key: ");
		for (m = 0; m < key_size; m++) {
			printf("%x", prev_key[m]);
		}
		printf("\n");
		// Take last column of prev_key
		for (m = 0; m < length; m++) {
			key_column[m] = prev_key[length * m + length - 1];
		}

		key_column[length] = '\0';

		for (m = 0; m < length; m++) {
			printf("%x", key_column[m]);
		}
		printf("\n");

		// Swap last block with first block
		swap = key_column[length - 1];
		key_column[length - 1] = key_column[0];
		key_column[0] = swap;

		for (m = 0; m < length; m++) {
			printf("%x", key_column[m]);
		}
		printf("\n");

		// Run each byte through sbox
		for (m = 0; m < length; m++) {
			key_column[m] = sbox(key_column[m]);
		}

		for (m = 0; m < length; m++) {
			printf("%x", key_column[m]);
		}
		printf("\n");

		// XOR with round constant
		for (m = 0; m < length; m++) {
			key_column[m] = key_column[m] ^ round_constants[i][m];
		}

		for (m = 0; m < length; m++) {
			printf("%x", key_column[m]);
		}
		printf("\n");

		// XOR with the first column of previous key
			for (m = 0; m < length; m++) {
			key_column[m] = key_column[m] ^ prev_key[length * m];
		}

		for (m = 0; m < length; m++) {
			printf("%x", key_column[m]);
		}
		printf("\n");

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

		// printf("\nRound key for %d: ", i);
		// for (m = 0; m < key_size - 1; m++) {
		// 	printf("%x", round_keys[i][m]);
		// }
		// printf("\n");
	}

	for (l = 0; l < rounds - 1; l++) {
		free(round_keys[l]);
	}
	free(key_column);
	free(key);
	free(prev_key);
}

void aes_dec(unsigned char *buf, int len) {
}
