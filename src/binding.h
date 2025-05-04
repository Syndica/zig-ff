#pragma once

#include <stdint.h>
#include <stdbool.h>

#define ulong unsigned long
#define uchar unsigned char

#define BN254_ALIGN (16UL)
#define BN254_G1_FOOTPRINT (64UL)
#define BN254_G2_FOOTPRINT (128UL)
#define BN254_G1_COMPRESSED_FOOTPRINT (32UL)
#define BN254_G2_COMPRESSED_FOOTPRINT (64UL)
#define BN254_BIGINT_FOOTPRINT (32UL)

struct bn254_point_g1 {
  __attribute__((aligned(BN254_ALIGN))) uchar v[BN254_G1_FOOTPRINT];
};
typedef struct bn254_point_g1 bn254_point_g1_t;

struct bn254_point_g2 {
  __attribute__((aligned(BN254_ALIGN))) uchar v[BN254_G2_FOOTPRINT];
};
typedef struct bn254_point_g2 bn254_point_g2_t;

struct bn254_point_g1_compressed {
  __attribute__((aligned(BN254_ALIGN))) uchar v[BN254_G1_COMPRESSED_FOOTPRINT];
};
typedef struct bn254_point_g1_compressed bn254_point_g1_compressed_t;

struct bn254_point_g2_compressed {
  __attribute__((aligned(BN254_ALIGN))) uchar v[BN254_G2_COMPRESSED_FOOTPRINT];
};
typedef struct bn254_point_g2_compressed bn254_point_g2_compressed_t;

struct bn254_bigint {
  __attribute__((aligned(BN254_ALIGN))) uchar v[BN254_BIGINT_FOOTPRINT];
};
typedef struct bn254_bigint bn254_bigint_t;

/* Return true if the point is on the curve */
int bn254_g1_check(bn254_point_g1_t const *p);

/* Returns true if the point is zero */
bool bn254_g1_is_zero(bn254_point_g1_t const *p);

const char* bn254_g1_print(bn254_point_g1_t const *p);

// /* Extract the X coordinate from the point */
// void bn254_g1_compress( bn254_point_g1_t const * in,
// bn254_point_g1_compressed_t * out );

// /* Recover the X,Y pair from X */
// void bn254_g1_decompress( bn254_point_g1_compressed_t const * in,
// bn254_point_g1_t * out );

/* Return true if the point is on the curve */
int bn254_g2_check(bn254_point_g2_t const *p);

/* Returns true if the point is zero */
bool bn254_g2_is_zero(bn254_point_g2_t const *p);

const char* bn254_g2_print(bn254_point_g2_t const *p);

// /* Extract the X coordinate from the point */
// void bn254_g2_compress( bn254_point_g2_t const * in,
// bn254_point_g2_compressed_t * out );

// /* Recover the X,Y pair from X */
// void bn254_g2_decompress( bn254_point_g2_compressed_t const * in,
// bn254_point_g2_t * out );

void bn254_g1_add(bn254_point_g1_t const *x, bn254_point_g1_t const *y,
                  bn254_point_g1_t *z);

void bn254_g1_mul(bn254_point_g1_t const *x, bn254_bigint_t const *y,
                  bn254_point_g1_t *z);

bool bn254_pairing(bn254_point_g1_t const *g1_1, bn254_point_g2_t const *g2_1, uint32_t num_elements);