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
// Lots of which is heavily inspired from https://github.com/jhjin/OpenAES/blob/master/src/oaes_lib.c
// Methods taken directly from oaes is named accordingly
int modInverse(int a, int m);
void gmix_column(unsigned char *r);
void inv_gmix_column(unsigned char *r);
void shift_rows(unsigned char *block);
void inv_shift_rows(unsigned char *block);
unsigned char sbox(unsigned char byte);
unsigned char inv_sbox(unsigned char byte);
/* Function prototypes. */
void vigenere(unsigned char *buf, int len);
void affine_enc(unsigned char *buf, int len);
void affine_dec(unsigned char *buf, int len);
void aes_enc(unsigned char *buf, int len);
void aes_dec(unsigned char *buf, int len);

uint8_t oaes_gf_mul(uint8_t left, uint8_t right);

static uint8_t oaes_gf_mul_2[16][16] = {
  //     0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
  /*0*/  0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e,
  /*1*/  0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e,
  /*2*/  0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e,
  /*3*/  0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e,
  /*4*/  0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e,
  /*5*/  0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe,
  /*6*/  0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde,
  /*7*/  0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0xfe,
  /*8*/  0x1b, 0x19, 0x1f, 0x1d, 0x13, 0x11, 0x17, 0x15, 0x0b, 0x09, 0x0f, 0x0d, 0x03, 0x01, 0x07, 0x05,
  /*9*/  0x3b, 0x39, 0x3f, 0x3d, 0x33, 0x31, 0x37, 0x35, 0x2b, 0x29, 0x2f, 0x2d, 0x23, 0x21, 0x27, 0x25,
  /*a*/  0x5b, 0x59, 0x5f, 0x5d, 0x53, 0x51, 0x57, 0x55, 0x4b, 0x49, 0x4f, 0x4d, 0x43, 0x41, 0x47, 0x45,
  /*b*/  0x7b, 0x79, 0x7f, 0x7d, 0x73, 0x71, 0x77, 0x75, 0x6b, 0x69, 0x6f, 0x6d, 0x63, 0x61, 0x67, 0x65,
  /*c*/  0x9b, 0x99, 0x9f, 0x9d, 0x93, 0x91, 0x97, 0x95, 0x8b, 0x89, 0x8f, 0x8d, 0x83, 0x81, 0x87, 0x85,
  /*d*/  0xbb, 0xb9, 0xbf, 0xbd, 0xb3, 0xb1, 0xb7, 0xb5, 0xab, 0xa9, 0xaf, 0xad, 0xa3, 0xa1, 0xa7, 0xa5,
  /*e*/  0xdb, 0xd9, 0xdf, 0xdd, 0xd3, 0xd1, 0xd7, 0xd5, 0xcb, 0xc9, 0xcf, 0xcd, 0xc3, 0xc1, 0xc7, 0xc5,
  /*f*/  0xfb, 0xf9, 0xff, 0xfd, 0xf3, 0xf1, 0xf7, 0xf5, 0xeb, 0xe9, 0xef, 0xed, 0xe3, 0xe1, 0xe7, 0xe5,
};

static uint8_t oaes_gf_mul_3[16][16] = {
  //     0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
  /*0*/  0x00, 0x03, 0x06, 0x05, 0x0c, 0x0f, 0x0a, 0x09, 0x18, 0x1b, 0x1e, 0x1d, 0x14, 0x17, 0x12, 0x11,
  /*1*/  0x30, 0x33, 0x36, 0x35, 0x3c, 0x3f, 0x3a, 0x39, 0x28, 0x2b, 0x2e, 0x2d, 0x24, 0x27, 0x22, 0x21,
  /*2*/  0x60, 0x63, 0x66, 0x65, 0x6c, 0x6f, 0x6a, 0x69, 0x78, 0x7b, 0x7e, 0x7d, 0x74, 0x77, 0x72, 0x71,
  /*3*/  0x50, 0x53, 0x56, 0x55, 0x5c, 0x5f, 0x5a, 0x59, 0x48, 0x4b, 0x4e, 0x4d, 0x44, 0x47, 0x42, 0x41,
  /*4*/  0xc0, 0xc3, 0xc6, 0xc5, 0xcc, 0xcf, 0xca, 0xc9, 0xd8, 0xdb, 0xde, 0xdd, 0xd4, 0xd7, 0xd2, 0xd1,
  /*5*/  0xf0, 0xf3, 0xf6, 0xf5, 0xfc, 0xff, 0xfa, 0xf9, 0xe8, 0xeb, 0xee, 0xed, 0xe4, 0xe7, 0xe2, 0xe1,
  /*6*/  0xa0, 0xa3, 0xa6, 0xa5, 0xac, 0xaf, 0xaa, 0xa9, 0xb8, 0xbb, 0xbe, 0xbd, 0xb4, 0xb7, 0xb2, 0xb1,
  /*7*/  0x90, 0x93, 0x96, 0x95, 0x9c, 0x9f, 0x9a, 0x99, 0x88, 0x8b, 0x8e, 0x8d, 0x84, 0x87, 0x82, 0x81,
  /*8*/  0x9b, 0x98, 0x9d, 0x9e, 0x97, 0x94, 0x91, 0x92, 0x83, 0x80, 0x85, 0x86, 0x8f, 0x8c, 0x89, 0x8a,
  /*9*/  0xab, 0xa8, 0xad, 0xae, 0xa7, 0xa4, 0xa1, 0xa2, 0xb3, 0xb0, 0xb5, 0xb6, 0xbf, 0xbc, 0xb9, 0xba,
  /*a*/  0xfb, 0xf8, 0xfd, 0xfe, 0xf7, 0xf4, 0xf1, 0xf2, 0xe3, 0xe0, 0xe5, 0xe6, 0xef, 0xec, 0xe9, 0xea,
  /*b*/  0xcb, 0xc8, 0xcd, 0xce, 0xc7, 0xc4, 0xc1, 0xc2, 0xd3, 0xd0, 0xd5, 0xd6, 0xdf, 0xdc, 0xd9, 0xda,
  /*c*/  0x5b, 0x58, 0x5d, 0x5e, 0x57, 0x54, 0x51, 0x52, 0x43, 0x40, 0x45, 0x46, 0x4f, 0x4c, 0x49, 0x4a,
  /*d*/  0x6b, 0x68, 0x6d, 0x6e, 0x67, 0x64, 0x61, 0x62, 0x73, 0x70, 0x75, 0x76, 0x7f, 0x7c, 0x79, 0x7a,
  /*e*/  0x3b, 0x38, 0x3d, 0x3e, 0x37, 0x34, 0x31, 0x32, 0x23, 0x20, 0x25, 0x26, 0x2f, 0x2c, 0x29, 0x2a,
  /*f*/  0x0b, 0x08, 0x0d, 0x0e, 0x07, 0x04, 0x01, 0x02, 0x13, 0x10, 0x15, 0x16, 0x1f, 0x1c, 0x19, 0x1a,
};

static uint8_t oaes_gf_mul_9[16][16] = {
  //     0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
  /*0*/  0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f, 0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
  /*1*/  0x90, 0x99, 0x82, 0x8b, 0xb4, 0xbd, 0xa6, 0xaf, 0xd8, 0xd1, 0xca, 0xc3, 0xfc, 0xf5, 0xee, 0xe7,
  /*2*/  0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04, 0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
  /*3*/  0xab, 0xa2, 0xb9, 0xb0, 0x8f, 0x86, 0x9d, 0x94, 0xe3, 0xea, 0xf1, 0xf8, 0xc7, 0xce, 0xd5, 0xdc,
  /*4*/  0x76, 0x7f, 0x64, 0x6d, 0x52, 0x5b, 0x40, 0x49, 0x3e, 0x37, 0x2c, 0x25, 0x1a, 0x13, 0x08, 0x01,
  /*5*/  0xe6, 0xef, 0xf4, 0xfd, 0xc2, 0xcb, 0xd0, 0xd9, 0xae, 0xa7, 0xbc, 0xb5, 0x8a, 0x83, 0x98, 0x91,
  /*6*/  0x4d, 0x44, 0x5f, 0x56, 0x69, 0x60, 0x7b, 0x72, 0x05, 0x0c, 0x17, 0x1e, 0x21, 0x28, 0x33, 0x3a,
  /*7*/  0xdd, 0xd4, 0xcf, 0xc6, 0xf9, 0xf0, 0xeb, 0xe2, 0x95, 0x9c, 0x87, 0x8e, 0xb1, 0xb8, 0xa3, 0xaa,
  /*8*/  0xec, 0xe5, 0xfe, 0xf7, 0xc8, 0xc1, 0xda, 0xd3, 0xa4, 0xad, 0xb6, 0xbf, 0x80, 0x89, 0x92, 0x9b,
  /*9*/  0x7c, 0x75, 0x6e, 0x67, 0x58, 0x51, 0x4a, 0x43, 0x34, 0x3d, 0x26, 0x2f, 0x10, 0x19, 0x02, 0x0b,
  /*a*/  0xd7, 0xde, 0xc5, 0xcc, 0xf3, 0xfa, 0xe1, 0xe8, 0x9f, 0x96, 0x8d, 0x84, 0xbb, 0xb2, 0xa9, 0xa0,
  /*b*/  0x47, 0x4e, 0x55, 0x5c, 0x63, 0x6a, 0x71, 0x78, 0x0f, 0x06, 0x1d, 0x14, 0x2b, 0x22, 0x39, 0x30,
  /*c*/  0x9a, 0x93, 0x88, 0x81, 0xbe, 0xb7, 0xac, 0xa5, 0xd2, 0xdb, 0xc0, 0xc9, 0xf6, 0xff, 0xe4, 0xed,
  /*d*/  0x0a, 0x03, 0x18, 0x11, 0x2e, 0x27, 0x3c, 0x35, 0x42, 0x4b, 0x50, 0x59, 0x66, 0x6f, 0x74, 0x7d,
  /*e*/  0xa1, 0xa8, 0xb3, 0xba, 0x85, 0x8c, 0x97, 0x9e, 0xe9, 0xe0, 0xfb, 0xf2, 0xcd, 0xc4, 0xdf, 0xd6,
  /*f*/  0x31, 0x38, 0x23, 0x2a, 0x15, 0x1c, 0x07, 0x0e, 0x79, 0x70, 0x6b, 0x62, 0x5d, 0x54, 0x4f, 0x46,
};

static uint8_t oaes_gf_mul_b[16][16] = {
  //     0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
  /*0*/  0x00, 0x0b, 0x16, 0x1d, 0x2c, 0x27, 0x3a, 0x31, 0x58, 0x53, 0x4e, 0x45, 0x74, 0x7f, 0x62, 0x69,
  /*1*/  0xb0, 0xbb, 0xa6, 0xad, 0x9c, 0x97, 0x8a, 0x81, 0xe8, 0xe3, 0xfe, 0xf5, 0xc4, 0xcf, 0xd2, 0xd9,
  /*2*/  0x7b, 0x70, 0x6d, 0x66, 0x57, 0x5c, 0x41, 0x4a, 0x23, 0x28, 0x35, 0x3e, 0x0f, 0x04, 0x19, 0x12,
  /*3*/  0xcb, 0xc0, 0xdd, 0xd6, 0xe7, 0xec, 0xf1, 0xfa, 0x93, 0x98, 0x85, 0x8e, 0xbf, 0xb4, 0xa9, 0xa2,
  /*4*/  0xf6, 0xfd, 0xe0, 0xeb, 0xda, 0xd1, 0xcc, 0xc7, 0xae, 0xa5, 0xb8, 0xb3, 0x82, 0x89, 0x94, 0x9f,
  /*5*/  0x46, 0x4d, 0x50, 0x5b, 0x6a, 0x61, 0x7c, 0x77, 0x1e, 0x15, 0x08, 0x03, 0x32, 0x39, 0x24, 0x2f,
  /*6*/  0x8d, 0x86, 0x9b, 0x90, 0xa1, 0xaa, 0xb7, 0xbc, 0xd5, 0xde, 0xc3, 0xc8, 0xf9, 0xf2, 0xef, 0xe4,
  /*7*/  0x3d, 0x36, 0x2b, 0x20, 0x11, 0x1a, 0x07, 0x0c, 0x65, 0x6e, 0x73, 0x78, 0x49, 0x42, 0x5f, 0x54,
  /*8*/  0xf7, 0xfc, 0xe1, 0xea, 0xdb, 0xd0, 0xcd, 0xc6, 0xaf, 0xa4, 0xb9, 0xb2, 0x83, 0x88, 0x95, 0x9e,
  /*9*/  0x47, 0x4c, 0x51, 0x5a, 0x6b, 0x60, 0x7d, 0x76, 0x1f, 0x14, 0x09, 0x02, 0x33, 0x38, 0x25, 0x2e,
  /*a*/  0x8c, 0x87, 0x9a, 0x91, 0xa0, 0xab, 0xb6, 0xbd, 0xd4, 0xdf, 0xc2, 0xc9, 0xf8, 0xf3, 0xee, 0xe5,
  /*b*/  0x3c, 0x37, 0x2a, 0x21, 0x10, 0x1b, 0x06, 0x0d, 0x64, 0x6f, 0x72, 0x79, 0x48, 0x43, 0x5e, 0x55,
  /*c*/  0x01, 0x0a, 0x17, 0x1c, 0x2d, 0x26, 0x3b, 0x30, 0x59, 0x52, 0x4f, 0x44, 0x75, 0x7e, 0x63, 0x68,
  /*d*/  0xb1, 0xba, 0xa7, 0xac, 0x9d, 0x96, 0x8b, 0x80, 0xe9, 0xe2, 0xff, 0xf4, 0xc5, 0xce, 0xd3, 0xd8,
  /*e*/  0x7a, 0x71, 0x6c, 0x67, 0x56, 0x5d, 0x40, 0x4b, 0x22, 0x29, 0x34, 0x3f, 0x0e, 0x05, 0x18, 0x13,
  /*f*/  0xca, 0xc1, 0xdc, 0xd7, 0xe6, 0xed, 0xf0, 0xfb, 0x92, 0x99, 0x84, 0x8f, 0xbe, 0xb5, 0xa8, 0xa3,
};

static uint8_t oaes_gf_mul_d[16][16] = {
  //     0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
  /*0*/  0x00, 0x0d, 0x1a, 0x17, 0x34, 0x39, 0x2e, 0x23, 0x68, 0x65, 0x72, 0x7f, 0x5c, 0x51, 0x46, 0x4b,
  /*1*/  0xd0, 0xdd, 0xca, 0xc7, 0xe4, 0xe9, 0xfe, 0xf3, 0xb8, 0xb5, 0xa2, 0xaf, 0x8c, 0x81, 0x96, 0x9b,
  /*2*/  0xbb, 0xb6, 0xa1, 0xac, 0x8f, 0x82, 0x95, 0x98, 0xd3, 0xde, 0xc9, 0xc4, 0xe7, 0xea, 0xfd, 0xf0,
  /*3*/  0x6b, 0x66, 0x71, 0x7c, 0x5f, 0x52, 0x45, 0x48, 0x03, 0x0e, 0x19, 0x14, 0x37, 0x3a, 0x2d, 0x20,
  /*4*/  0x6d, 0x60, 0x77, 0x7a, 0x59, 0x54, 0x43, 0x4e, 0x05, 0x08, 0x1f, 0x12, 0x31, 0x3c, 0x2b, 0x26,
  /*5*/  0xbd, 0xb0, 0xa7, 0xaa, 0x89, 0x84, 0x93, 0x9e, 0xd5, 0xd8, 0xcf, 0xc2, 0xe1, 0xec, 0xfb, 0xf6,
  /*6*/  0xd6, 0xdb, 0xcc, 0xc1, 0xe2, 0xef, 0xf8, 0xf5, 0xbe, 0xb3, 0xa4, 0xa9, 0x8a, 0x87, 0x90, 0x9d,
  /*7*/  0x06, 0x0b, 0x1c, 0x11, 0x32, 0x3f, 0x28, 0x25, 0x6e, 0x63, 0x74, 0x79, 0x5a, 0x57, 0x40, 0x4d,
  /*8*/  0xda, 0xd7, 0xc0, 0xcd, 0xee, 0xe3, 0xf4, 0xf9, 0xb2, 0xbf, 0xa8, 0xa5, 0x86, 0x8b, 0x9c, 0x91,
  /*9*/  0x0a, 0x07, 0x10, 0x1d, 0x3e, 0x33, 0x24, 0x29, 0x62, 0x6f, 0x78, 0x75, 0x56, 0x5b, 0x4c, 0x41,
  /*a*/  0x61, 0x6c, 0x7b, 0x76, 0x55, 0x58, 0x4f, 0x42, 0x09, 0x04, 0x13, 0x1e, 0x3d, 0x30, 0x27, 0x2a,
  /*b*/  0xb1, 0xbc, 0xab, 0xa6, 0x85, 0x88, 0x9f, 0x92, 0xd9, 0xd4, 0xc3, 0xce, 0xed, 0xe0, 0xf7, 0xfa,
  /*c*/  0xb7, 0xba, 0xad, 0xa0, 0x83, 0x8e, 0x99, 0x94, 0xdf, 0xd2, 0xc5, 0xc8, 0xeb, 0xe6, 0xf1, 0xfc,
  /*d*/  0x67, 0x6a, 0x7d, 0x70, 0x53, 0x5e, 0x49, 0x44, 0x0f, 0x02, 0x15, 0x18, 0x3b, 0x36, 0x21, 0x2c,
  /*e*/  0x0c, 0x01, 0x16, 0x1b, 0x38, 0x35, 0x22, 0x2f, 0x64, 0x69, 0x7e, 0x73, 0x50, 0x5d, 0x4a, 0x47,
  /*f*/  0xdc, 0xd1, 0xc6, 0xcb, 0xe8, 0xe5, 0xf2, 0xff, 0xb4, 0xb9, 0xae, 0xa3, 0x80, 0x8d, 0x9a, 0x97,
};

static uint8_t oaes_gf_mul_e[16][16] = {
  //     0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
  /*0*/  0x00, 0x0e, 0x1c, 0x12, 0x38, 0x36, 0x24, 0x2a, 0x70, 0x7e, 0x6c, 0x62, 0x48, 0x46, 0x54, 0x5a,
  /*1*/  0xe0, 0xee, 0xfc, 0xf2, 0xd8, 0xd6, 0xc4, 0xca, 0x90, 0x9e, 0x8c, 0x82, 0xa8, 0xa6, 0xb4, 0xba,
  /*2*/  0xdb, 0xd5, 0xc7, 0xc9, 0xe3, 0xed, 0xff, 0xf1, 0xab, 0xa5, 0xb7, 0xb9, 0x93, 0x9d, 0x8f, 0x81,
  /*3*/  0x3b, 0x35, 0x27, 0x29, 0x03, 0x0d, 0x1f, 0x11, 0x4b, 0x45, 0x57, 0x59, 0x73, 0x7d, 0x6f, 0x61,
  /*4*/  0xad, 0xa3, 0xb1, 0xbf, 0x95, 0x9b, 0x89, 0x87, 0xdd, 0xd3, 0xc1, 0xcf, 0xe5, 0xeb, 0xf9, 0xf7,
  /*5*/  0x4d, 0x43, 0x51, 0x5f, 0x75, 0x7b, 0x69, 0x67, 0x3d, 0x33, 0x21, 0x2f, 0x05, 0x0b, 0x19, 0x17,
  /*6*/  0x76, 0x78, 0x6a, 0x64, 0x4e, 0x40, 0x52, 0x5c, 0x06, 0x08, 0x1a, 0x14, 0x3e, 0x30, 0x22, 0x2c,
  /*7*/  0x96, 0x98, 0x8a, 0x84, 0xae, 0xa0, 0xb2, 0xbc, 0xe6, 0xe8, 0xfa, 0xf4, 0xde, 0xd0, 0xc2, 0xcc,
  /*8*/  0x41, 0x4f, 0x5d, 0x53, 0x79, 0x77, 0x65, 0x6b, 0x31, 0x3f, 0x2d, 0x23, 0x09, 0x07, 0x15, 0x1b,
  /*9*/  0xa1, 0xaf, 0xbd, 0xb3, 0x99, 0x97, 0x85, 0x8b, 0xd1, 0xdf, 0xcd, 0xc3, 0xe9, 0xe7, 0xf5, 0xfb,
  /*a*/  0x9a, 0x94, 0x86, 0x88, 0xa2, 0xac, 0xbe, 0xb0, 0xea, 0xe4, 0xf6, 0xf8, 0xd2, 0xdc, 0xce, 0xc0,
  /*b*/  0x7a, 0x74, 0x66, 0x68, 0x42, 0x4c, 0x5e, 0x50, 0x0a, 0x04, 0x16, 0x18, 0x32, 0x3c, 0x2e, 0x20,
  /*c*/  0xec, 0xe2, 0xf0, 0xfe, 0xd4, 0xda, 0xc8, 0xc6, 0x9c, 0x92, 0x80, 0x8e, 0xa4, 0xaa, 0xb8, 0xb6,
  /*d*/  0x0c, 0x02, 0x10, 0x1e, 0x34, 0x3a, 0x28, 0x26, 0x7c, 0x72, 0x60, 0x6e, 0x44, 0x4a, 0x58, 0x56,
  /*e*/  0x37, 0x39, 0x2b, 0x25, 0x0f, 0x01, 0x13, 0x1d, 0x47, 0x49, 0x5b, 0x55, 0x7f, 0x71, 0x63, 0x6d,
  /*f*/  0xd7, 0xd9, 0xcb, 0xc5, 0xef, 0xe1, 0xf3, 0xfd, 0xa7, 0xa9, 0xbb, 0xb5, 0x9f, 0x91, 0x83, 0x8d,
};

static uint8_t oaes_sub_byte_value[16][16] = {
  //     0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
  /*0*/  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  /*1*/  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  /*2*/  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  /*3*/  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  /*4*/  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  /*5*/  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  /*6*/  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  /*7*/  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  /*8*/  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  /*9*/  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  /*a*/  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  /*b*/  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  /*c*/  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  /*d*/  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  /*e*/  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  /*f*/  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
};

static uint8_t oaes_inv_sub_byte_value[16][16] = {
  //     0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
  /*0*/  0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
  /*1*/  0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
  /*2*/  0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
  /*3*/  0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
  /*4*/  0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
  /*5*/  0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
  /*6*/  0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
  /*7*/  0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
  /*8*/  0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
  /*9*/  0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
  /*a*/  0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
  /*b*/  0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
  /*c*/  0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
  /*d*/  0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
  /*e*/  0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
  /*f*/  0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d,
};

static unsigned char initialization_vector[16] = {
	0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb
};

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
	unsigned char block[block_size];
	unsigned char prev_block[block_size];
	unsigned int bytes;
	unsigned char byte, swap;
	unsigned int key_size = (length*sizeof(uint32_t)) * (sizeof(unsigned char));
	unsigned int column_size = (length) * (sizeof(unsigned char));
	unsigned char key_column[column_size];
	unsigned char block_row[column_size];
	unsigned char block_column[column_size];
	unsigned char key[key_size];
	unsigned char prev_key[key_size];
	unsigned char round_keys[rounds][key_size];
	unsigned char rc, prev;
	unsigned char round_constants[rounds][sizeof(uint32_t)];

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
			memcpy(prev_key, key, key_size);
		} else {
			// Otherwise fill prev_key with previous round key
			memcpy(prev_key, round_keys[i - 1], key_size);
		}

		// Take last column of prev_key
		for (m = 0; m < length; m++) {
			key_column[m] = prev_key[length * m + length - 1];
		}

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
				round_keys[i][length * r + m] = prev_key[length * r + m] ^ round_keys[i][length * r + (m - 1)];
			}
		}

	}


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

			// If CBC mode of operation uncomment this
			// If first block use IV
			// if (j == 0) {
			// 	for (m = 0; m < block_size; m++)
			// 		block[m] = block[m] ^ initialization_vector[m];
			// } else {
			// 	// Other wise use previous block
			// 	for (m = 0; m < block_size; m++)
			// 		block[m] = block[m] ^ prev_block[m];
			// }

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

				// Apply diffusion - shift rows
				shift_rows(block);

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

			// Scoop block into buf again - ECB
			for (m = 0; m < block_size; m++) {
					buf[j + m] = block[m];
			}

			// Save into prev_block for CBC mode
			memcpy(prev_block, block, block_size);
		}
}

void aes_dec(unsigned char *buf, int len) {
	int i, j, k, l, m, n, r;
	int length = 4;
	unsigned int rounds = 9;
	unsigned int block_size = 16;
	unsigned char block[block_size];
	unsigned char curr_block[block_size];
	unsigned char prev_block[block_size];
	unsigned int bytes;
	unsigned char byte, swap;
	unsigned int key_size = 16;
	unsigned int column_size = (length) * (sizeof(unsigned char));
	unsigned char key_column[column_size];
	unsigned char block_row[column_size];
	unsigned char block_column[column_size];
	unsigned char key[key_size];
	unsigned char prev_key[key_size];
	unsigned char round_keys[rounds][key_size];
	unsigned char rc, prev;
	unsigned char round_constants[rounds][sizeof(uint32_t)];

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
			memcpy(prev_key, key, key_size);
		} else {
			// Otherwise fill prev_key with previous round key
			memcpy(prev_key, round_keys[i - 1], key_size);
		}

		// Take last column of prev_key
		for (m = 0; m < length; m++) {
			key_column[m] = prev_key[length * m + length - 1];
		}

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
				round_keys[i][length * r + m] = prev_key[length * r + m] ^ round_keys[i][length * r + (m - 1)];
			}
		}
	}

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

			// Save into curr_block for CBC mode
			memcpy(curr_block, block, block_size);

			// Intermediate rounds
			for (n = rounds - 1; n >= 0; n--) {

				// Apply key secrecy
				for (k = 0; k < block_size; k++) {
					block[k] = block[k] ^ round_keys[n][k];
				}

				// Mix columns
				// Only if it is not the first round
				if (n < (rounds - 1)) {
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

				// Apply diffusion - shift rows
				inv_shift_rows(block);

				// Apply confusion
				for (m = 0; m < block_size; m++) {
					block[m] = inv_sbox(block[m]);
				}
			}

			// Last round - just XOR input with initial key
			for (m = 0; m < block_size; m++) {
				block[m] = block[m] ^ key[m];
			}

			// If CBC mode of operation uncomment this
			// If first block use IV
			// if (j == 0) {
			// 	for (m = 0; m < block_size; m++)
			// 		block[m] = block[m] ^ initialization_vector[m];
			// } else {
			// 	// Otherwise use previous block
			// 	for (m = 0; m < block_size; m++)
			// 		block[m] = block[m] ^ prev_block[m];
			// }

			// Scoop block into buf again - ECB
			for (m = 0; m < block_size; m++) {
					buf[j + m] = block[m];
			}

			// Save into prev_block for CBC mode
			memcpy(prev_block, curr_block, block_size);
		}
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
	size_t _x, _y;

  _x = _y = byte;
  _x &= 0x0f;
  _y &= 0xf0;
  _y >>= 4;
  return oaes_sub_byte_value[_y][_x];
}

// From https://en.wikipedia.org/wiki/Rijndael_S-box
unsigned char inv_sbox(unsigned char byte) {
	size_t _x, _y;

  _x = _y = byte;
  _x &= 0x0f;
  _y &= 0xf0;
  _y >>= 4;
  return oaes_inv_sub_byte_value[_y][_x];
}

void shift_rows(unsigned char *block) {
	unsigned int i;
	unsigned int block_size = 16;
	unsigned char _temp[block_size];

	_temp[0x00] = block[0x00];
  _temp[0x01] = block[0x01];
  _temp[0x02] = block[0x02];
  _temp[0x03] = block[0x03];
  _temp[0x04] = block[0x05];
  _temp[0x05] = block[0x06];
  _temp[0x06] = block[0x07];
  _temp[0x07] = block[0x04];
  _temp[0x08] = block[0x0a];
  _temp[0x09] = block[0x0b];
  _temp[0x0a] = block[0x08];
  _temp[0x0b] = block[0x09];
  _temp[0x0c] = block[0x0f];
  _temp[0x0d] = block[0x0c];
  _temp[0x0e] = block[0x0d];
  _temp[0x0f] = block[0x0e];
	memcpy(block, _temp, block_size);
}
void inv_shift_rows(unsigned char *block) {
	unsigned int i;
	unsigned int block_size = 16;
	unsigned char _temp[block_size];

	_temp[0x00] = block[0x00];
  _temp[0x01] = block[0x01];
  _temp[0x02] = block[0x02];
  _temp[0x03] = block[0x03];
  _temp[0x04] = block[0x07];
  _temp[0x05] = block[0x04];
  _temp[0x06] = block[0x05];
  _temp[0x07] = block[0x06];
  _temp[0x08] = block[0x0a];
  _temp[0x09] = block[0x0b];
  _temp[0x0a] = block[0x08];
  _temp[0x0b] = block[0x09];
  _temp[0x0c] = block[0x0d];
  _temp[0x0d] = block[0x0e];
  _temp[0x0e] = block[0x0f];
  _temp[0x0f] = block[0x0c];
	memcpy(block, _temp, block_size);
}

// From https://en.wikipedia.org/wiki/Rijndael_MixColumns
void gmix_column(unsigned char *r) {
	unsigned int block_size = 16;
	int8_t _temp[4];

  _temp[0] = oaes_gf_mul(r[0], 0x02) ^ oaes_gf_mul( r[1], 0x03 ) ^ r[2] ^ r[3];
  _temp[1] = r[0] ^ oaes_gf_mul( r[1], 0x02 ) ^ oaes_gf_mul( r[2], 0x03 ) ^ r[3];
  _temp[2] = r[0] ^ r[1] ^ oaes_gf_mul( r[2], 0x02 ) ^ oaes_gf_mul( r[3], 0x03 );
  _temp[3] = oaes_gf_mul( r[0], 0x03 ) ^ r[1] ^ r[2] ^ oaes_gf_mul( r[3], 0x02 );
  memcpy( r, _temp, 4 );
}

void inv_gmix_column(unsigned char *r) {
	int8_t _temp[4];
  
  _temp[0] = oaes_gf_mul( r[0], 0x0e ) ^ oaes_gf_mul( r[1], 0x0b ) ^ oaes_gf_mul( r[2], 0x0d ) ^ oaes_gf_mul( r[3], 0x09 );
  _temp[1] = oaes_gf_mul( r[0], 0x09 ) ^ oaes_gf_mul( r[1], 0x0e ) ^ oaes_gf_mul( r[2], 0x0b ) ^ oaes_gf_mul( r[3], 0x0d );
  _temp[2] = oaes_gf_mul( r[0], 0x0d ) ^ oaes_gf_mul( r[1], 0x09 ) ^ oaes_gf_mul( r[2], 0x0e ) ^ oaes_gf_mul( r[3], 0x0b );
  _temp[3] = oaes_gf_mul( r[0], 0x0b ) ^ oaes_gf_mul( r[1], 0x0d ) ^ oaes_gf_mul( r[2], 0x09 ) ^ oaes_gf_mul( r[3], 0x0e );
  memcpy( r, _temp, 4 );
}

uint8_t oaes_gf_mul(uint8_t left, uint8_t right) {
  size_t _x, _y;
  
  _x = _y = left;
  _x &= 0x0f;
  _y &= 0xf0;
  _y >>= 4;
  
  switch( right )
  {
    case 0x02:
      return oaes_gf_mul_2[_y][_x];
      break;
    case 0x03:
      return oaes_gf_mul_3[_y][_x];
      break;
    case 0x09:
      return oaes_gf_mul_9[_y][_x];
      break;
    case 0x0b:
      return oaes_gf_mul_b[_y][_x];
      break;
    case 0x0d:
      return oaes_gf_mul_d[_y][_x];
      break;
    case 0x0e:
      return oaes_gf_mul_e[_y][_x];
      break;
    default:
      return left;
      break;
  }
}