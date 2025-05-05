extern "C" {
#include "binding.h"
}

#include <libff/algebra/curves/alt_bn128/alt_bn128_fields.hpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_g1.hpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_g2.hpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_init.hpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_pairing.hpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_pp.hpp>

#define BN254_FIELD_FOOTPRINT 32

static bool initialized = false;

#define FLAG_INF ((uint8_t)(1 << 6))
#define FLAG_NEG ((uint8_t)(1 << 7))
#define FLAG_MASK 0x3F

static libff::alt_bn128_Fq *bytes_to_Fq(uint8_t const input[32],
                                        libff::alt_bn128_Fq *X, int *is_inf,
                                        int *is_neg) {
  if (is_inf != NULL) {
    *is_inf = !!(input[0] & FLAG_INF);
    *is_neg = !!(input[0] & FLAG_NEG);

    // Return an error if both flags are set,
    // https://github.com/arkworks-rs/algebra/blob/v0.4.2/ec/src/models/short_weierstrass/serialization_flags.rs#L75
    if (*is_inf && *is_neg) {
      return NULL;
    }
  }

  libff::bigint<libff::alt_bn128_r_limbs> bi;
  static_assert(sizeof(bi.data) == BN254_FIELD_FOOTPRINT);

  /* Convert big-endian to little-endian while copying */
  uint8_t *t = (uint8_t *)(bi.data);
  for (uint64_t i = 0; i < BN254_FIELD_FOOTPRINT; ++i)
    t[BN254_FIELD_FOOTPRINT - 1U - i] = input[i];

  if (is_inf != NULL) {
    t[31] &= FLAG_MASK;
  }

  // Check that it's a valid field element.
  if (!(bi < libff::alt_bn128_modulus_q)) {
    return NULL;
  }

  *X = libff::alt_bn128_Fq(bi);
  return X;
}

static libff::alt_bn128_G1 *bytes_to_G1_internal(uint8_t const input[64],
                                                 libff::alt_bn128_G1 *out) {
  // Special case, all zeroes => point at infinity.
  const uint8_t zero[64] = {0};
  if (memcmp(input, zero, 64) == 0) {
    out->Z = libff::alt_bn128_Fq::zero();
    return out;
  }

  // Check x < p
  if (!bytes_to_Fq(input, &out->X, NULL, NULL)) {
    return NULL;
  }

  // Check flags and y < p
  int is_inf, is_neg;
  if (!bytes_to_Fq(&input[32], &out->Y, &is_inf, &is_neg)) {
    return NULL;
  }

  out->Z = is_inf ? libff::alt_bn128_Fq::zero() : libff::alt_bn128_Fq::one();
  return out;
}

static libff::alt_bn128_G1 *bytes_to_G1(uint8_t const input[64],
                                        libff::alt_bn128_G1 *p) {
  if (!bytes_to_G1_internal(input, p)) {
    return NULL;
  }
  if (p->is_zero()) {
    return p;
  }

  if (!p->is_well_formed()) {
    return NULL;
  }
  return p;
}

static libff::alt_bn128_Fq2 *bytes_to_Fq2(uint8_t const input[64],
                                          libff::alt_bn128_Fq2 *out,
                                          int *is_inf, int *is_neg) {
  if (!bytes_to_Fq(input + 32, &out->c0, NULL, NULL)) {
    return NULL;
  }

  if (!bytes_to_Fq(input, &out->c1, is_inf, is_neg)) {
    return NULL;
  }

  return out;
}

static libff::alt_bn128_G2 *bytes_to_G2_internal(uint8_t const input[128],
                                                 libff::alt_bn128_G2 *out) {

  const uint8_t zero[128] = {0};
  if (memcmp(input, zero, 128) == 0) {
    out->Z = libff::alt_bn128_Fq2::zero();
  }

  // Check x < p
  if (!bytes_to_Fq2(input, &out->X, NULL, NULL)) {
    return NULL;
  }

  // Check flags and y < p
  int is_inf, is_neg;
  if (!bytes_to_Fq2(input + 64, &out->Y, &is_inf, &is_neg)) {
    return NULL;
  }

  out->Z = is_inf ? libff::alt_bn128_Fq2::zero() : libff::alt_bn128_Fq2::one();
  return out;
}

static libff::alt_bn128_G2 *bytes_to_G2(uint8_t const input[128],
                                        libff::alt_bn128_G2 *p) {
  if (!bytes_to_G2_internal(input, p)) {
    return NULL;
  }
  if (p->is_zero()) {
    return p;
  }

  if (!p->is_well_formed()) {
    return NULL;
  }
  return p;
}

static void Fq_to_bytes(libff::alt_bn128_Fq const *X, uint8_t *out) {
  libff::bigint<libff::alt_bn128_r_limbs> bi;
  static_assert(sizeof(bi.data) == BN254_FIELD_FOOTPRINT);
  bi = X->as_bigint();
  /* Convert little-endian to big-endian while copying */
  const uint8_t *t = (const uint8_t *)bi.data;
  for (uint64_t i = 0; i < BN254_FIELD_FOOTPRINT; ++i)
    out[i] = t[BN254_FIELD_FOOTPRINT - 1U - i];
}

static uint8_t *G1_to_bytes(libff::alt_bn128_G1 g, uint8_t out[64]) {
  if (g.is_zero()) {
    memset(out, 0, 64UL);
    return out;
  }

  g.to_affine_coordinates();
  Fq_to_bytes(&g.X, out);
  Fq_to_bytes(&g.Y, out + BN254_FIELD_FOOTPRINT);
  return out;
}

int bn254_add_syscall(uint8_t const *__restrict input,
                      uint8_t *__restrict out) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }

  libff::alt_bn128_G1 X;
  libff::alt_bn128_G1 Y;

  // Validate the inputs.
  if (!bytes_to_G1(input, &X))
    return -1;
  if (!bytes_to_G1(input + BN254_G1_FOOTPRINT, &Y))
    return -1;

  auto result = X + Y;

  G1_to_bytes(result, out);
  return 0;
}

int bn254_mul_syscall(uint8_t const *__restrict input,
                      uint8_t *__restrict out) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }

  libff::alt_bn128_G1 A;

  if (!bytes_to_G1(input, &A))
    return -1;

  /* Convert big-endian to little-endian while copying */
  libff::bigint<libff::alt_bn128_r_limbs> s;
  static_assert(sizeof(s.data) == BN254_BIGINT_FOOTPRINT);
  uint8_t *t = (uint8_t *)(s.data);
  for (uint64_t i = 0; i < BN254_BIGINT_FOOTPRINT; ++i)
    t[BN254_BIGINT_FOOTPRINT - 1U - i] = input[64 + i];

  auto result = s * A;
  G1_to_bytes(result, out);
  return 0;
}

int bn254_pairing_syscall(uint8_t const *__restrict input, uintptr_t input_len,
                          uint8_t *__restrict out) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }
  libff::inhibit_profiling_info = true;

  if (input_len % 192 != 0)
    return -1;
  auto element_length = input_len / 192;

  // TODO: use more parallel miller loop

  libff::alt_bn128_GT tmp = libff::alt_bn128_GT::one();
  for (uint64_t i = 0; i < element_length; i++) {
    libff::alt_bn128_G1 A;
    libff::alt_bn128_G2 B;

    if (!bytes_to_G1(&input[192 * i], &A))
      return -1;
    if (!bytes_to_G2(&input[192 * i + 64], &B))
      return -1;

    // Skip any pair where either A or B are points at infinity.
    if (A.is_zero() || B.is_zero())
      continue;

    tmp *= libff::alt_bn128_ate_pairing(A, B);
  }

  auto result = libff::alt_bn128_final_exponentiation(tmp);
  memset(out, 0, 32);
  out[31] = result == libff::alt_bn128_GT::one();
  return 0;
}

int bn254_compress_g1_syscall(uint8_t const *__restrict input,
                              uint8_t *__restrict out) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }

  libff::alt_bn128_G1 P;
  if (!bytes_to_G1(input, &P))
    return -1;

  uint8_t is_inf = P.is_zero();
  uint8_t flag_inf = input[32] & FLAG_INF;

  /*
    1. If the infinity flag is set, return point at infinity
    2. Else, copy x and set the neg_y flag
  */

  if (is_inf) {
    memset(out, 0, 32);
    out[0] |= flag_inf;
    return 0;
  }

  int is_neg = !(P.Y.as_bigint() < libff::alt_bn128_Fq::euler);
  memmove(out, input, 32);
  if (is_neg) {
    out[0] = (uint8_t)(out[0] | FLAG_NEG);
  }
  return 0;
}

int bn254_decompress_g1_syscall(uint8_t const *__restrict input,
                                uint8_t *__restrict out) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }

  libff::alt_bn128_Fq X;
  int is_inf, flag_neg;
  if (!bytes_to_Fq(input, &X, &is_inf, &flag_neg)) {
    return -1;
  }

  // If the point at infinity flag is set, return the point without checking
  // any of the coordinates.
  if (is_inf) {
    memset(out, 0, 64UL);
    return 0;
  }

  /*
    Recover Y coordinate from X
    Y^2 = X^3 + 3
  */
  libff::alt_bn128_Fq X2(X);
  X2.square();
  libff::alt_bn128_Fq X3_plus_b = X * X2 + libff::alt_bn128_coeff_b;
  auto root = X3_plus_b.sqrt();
  if (root == std::nullopt) {
    return -1;
  }
  libff::alt_bn128_Fq Y(*root);

  int is_neg = !(Y.as_bigint() < libff::alt_bn128_Fq::euler);
  if (flag_neg != is_neg) {
    Y = -Y;
  }

  memmove(out, input, 32);
  out[0] &= FLAG_MASK;
  Fq_to_bytes(&Y, out + 32);
  return 0;
}

bool Fq2_is_neg(libff::alt_bn128_Fq2 x) {
  if (x.c1.is_zero()) {
    return !(x.c0.as_bigint() < libff::alt_bn128_Fq::euler);
  }
  return !(x.c1.as_bigint() < libff::alt_bn128_Fq::euler);
}

int bn254_compress_g2_syscall(uint8_t const *__restrict input,
                              uint8_t *__restrict out) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }

  libff::alt_bn128_G2 P;
  if (!bytes_to_G2(input, &P))
    return -1;

  uint8_t is_inf = P.is_zero();
  uint8_t flag_inf = input[64] & FLAG_INF;

  if (is_inf) {
    memset(out, 0, 64);
    out[0] |= flag_inf;
    return 0;
  }

  bool is_neg = Fq2_is_neg(P.Y);
  memmove(out, input, 64);
  if (is_neg) {
    out[0] |= FLAG_NEG;
  }
  return 0;
}

int bn254_decompress_g2_syscall(uint8_t const *__restrict input,
                                uint8_t *__restrict out) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }

  const uint8_t zero[64] = {0};
  if (memcmp(input, zero, 64) == 0) {
    memset(out, 0, 128UL);
    return 0;
  }

  libff::alt_bn128_Fq2 X;
  int is_inf, flag_neg;
  if (!bytes_to_Fq2(input, &X, &is_inf, &flag_neg)) {
    return -1;
  }

  // If the point at infinity flag is set, return the point without checking
  // any of the coordinates.
  if (is_inf) {
    memset(out, 0, 128UL);
    return 0;
  }

  /*
    Recover Y coordinate from X
    Y^2 = X^3 + B
  */
  libff::alt_bn128_Fq2 X2(X);
  X2.square();
  libff::alt_bn128_Fq2 X3_plus_b = X * X2 + libff::alt_bn128_twist_coeff_b;
  auto root = X3_plus_b.sqrt();
  if (root == std::nullopt) {
    return -1;
  }
  libff::alt_bn128_Fq2 Y(*root);
  
  int is_neg = Fq2_is_neg(Y);
  if (flag_neg != is_neg) {
    Y = -Y;
  }

  memmove(out, input, 64);
  out[0] &= FLAG_MASK;
  Fq_to_bytes(&Y.c1, out + 64);
  Fq_to_bytes(&Y.c0, out + 64 + 32);
  return 0;
}
