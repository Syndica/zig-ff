//! Nicely inspired from
//! https://github.com/firedancer-io/firedancer/blob/21502277abc6911fef2546d70527fdccefe1c0a5/src/ballet/bn254/bn254.h

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

static void bn254_Fq_sol_to_libff(uchar const *sol, libff::alt_bn128_Fq *X) {
  libff::bigint<libff::alt_bn128_r_limbs> bi;
  static_assert(sizeof(bi.data) == BN254_FIELD_FOOTPRINT);
  /* Convert big-endian to little-endian while copying */
  uchar *t = (uchar *)(bi.data);
  for (ulong i = 0; i < BN254_FIELD_FOOTPRINT; ++i)
    t[BN254_FIELD_FOOTPRINT - 1U - i] = sol[i];
  /* Set inf and neg flags to false */
  t[BN254_FIELD_FOOTPRINT - 1U] &= (uchar)(~((1 << 6) | (1 << 7)));
  new (X) libff::alt_bn128_Fq(bi);
}

static void bn254_Fq_libff_to_sol(libff::alt_bn128_Fq const *X, uchar *sol) {
  libff::bigint<libff::alt_bn128_r_limbs> bi;
  static_assert(sizeof(bi.data) == BN254_FIELD_FOOTPRINT);
  bi = X->as_bigint();
  /* Convert little-endian to big-endian while copying */
  const uchar *t = (const uchar *)bi.data;
  for (ulong i = 0; i < BN254_FIELD_FOOTPRINT; ++i)
    sol[i] = t[BN254_FIELD_FOOTPRINT - 1U - i];
}

static libff::alt_bn128_G1 bn254_g1_sol_to_libff(bn254_point_g1_t const *p) {
  libff::alt_bn128_Fq X;
  bn254_Fq_sol_to_libff(p->v, &X);
  libff::alt_bn128_Fq Y;
  bn254_Fq_sol_to_libff(p->v + BN254_FIELD_FOOTPRINT, &Y);
  return libff::alt_bn128_G1(X, Y, libff::alt_bn128_Fq::one());
}

static void bn254_g1_libff_to_sol(libff::alt_bn128_G1 g, bn254_point_g1_t *p) {
  g.to_affine_coordinates();
  bn254_Fq_libff_to_sol(&g.X, p->v);
  bn254_Fq_libff_to_sol(&g.Y, p->v + BN254_FIELD_FOOTPRINT);
}

static libff::bigint<libff::alt_bn128_r_limbs>
bn254_bigint_sol_to_libff(bn254_bigint_t const *sol) {
  libff::bigint<libff::alt_bn128_r_limbs> bi;
  static_assert(sizeof(bi.data) == BN254_BIGINT_FOOTPRINT);
  /* Convert big-endian to little-endian while copying */
  uchar *t = (uchar *)(bi.data);
  for (ulong i = 0; i < BN254_FIELD_FOOTPRINT; ++i)
    t[BN254_BIGINT_FOOTPRINT - 1U - i] = sol->v[i];
  return bi;
}

int bn254_g1_check(bn254_point_g1_t const *p) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }
  return bn254_g1_sol_to_libff(p).is_well_formed();
}

bool bn254_g1_is_zero(bn254_point_g1_t const *p) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }
  return bn254_g1_sol_to_libff(p).is_zero();
}

const char* bn254_g1_print(bn254_point_g1_t const *p) {
  std::ostringstream oss;
  oss << bn254_g1_sol_to_libff(p);
  std::string str = oss.str();
  char* cstr = new char[str.size() + 1];
  std::strcpy(cstr, str.c_str());
  return cstr;
}

void bn254_g1_add(bn254_point_g1_t const *x, bn254_point_g1_t const *y,
                  bn254_point_g1_t *z) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }
  bn254_g1_libff_to_sol(bn254_g1_sol_to_libff(x) + bn254_g1_sol_to_libff(y), z);
}

void bn254_g1_mul(bn254_point_g1_t const *x, bn254_bigint_t const *y,
                  bn254_point_g1_t *z) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }
  bn254_g1_libff_to_sol(bn254_bigint_sol_to_libff(y) * bn254_g1_sol_to_libff(x),
                        z);
}

static void bn254_Fq2_sol_to_libff(uchar const *sol, libff::alt_bn128_Fq2 *X) {
  bn254_Fq_sol_to_libff(sol, &X->c1);
  bn254_Fq_sol_to_libff(sol + BN254_FIELD_FOOTPRINT, &X->c0);
}

static libff::alt_bn128_G2 bn254_g2_sol_to_libff(bn254_point_g2_t const *p) {
  libff::alt_bn128_Fq2 X;
  bn254_Fq2_sol_to_libff(p->v, &X);
  libff::alt_bn128_Fq2 Y;
  bn254_Fq2_sol_to_libff(p->v + 2U * BN254_FIELD_FOOTPRINT, &Y);
  return libff::alt_bn128_G2(X, Y, libff::alt_bn128_Fq2::one());
}

int bn254_g2_check(bn254_point_g2_t const *p) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }
  return bn254_g2_sol_to_libff(p).is_well_formed();
}

const char* bn254_g2_print(bn254_point_g2_t const *p) {
  std::ostringstream oss;
  oss << bn254_g2_sol_to_libff(p);
  std::string str = oss.str();
  char* cstr = new char[str.size() + 1];
  std::strcpy(cstr, str.c_str());
  return cstr;
}

bool bn254_g2_is_zero(bn254_point_g2_t const *p) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }
  return bn254_g2_sol_to_libff(p).is_zero();
}

bool bn254_pairing(bn254_point_g1_t const *p, bn254_point_g2_t const *q, uint32_t num_elements) {
  if (!initialized) {
    libff::init_alt_bn128_params();
    initialized = true;
  }
  libff::inhibit_profiling_info = true;

  libff::alt_bn128_GT tmp = libff::alt_bn128_GT::one();
  for (int i = 0; i < num_elements; i++) {
    auto a = p[i];
    auto b = q[i];
    tmp *= libff::alt_bn128_ate_pairing(
      bn254_g1_sol_to_libff(&a),
      bn254_g2_sol_to_libff(&b)
    );  
  }
  auto result = libff::alt_bn128_final_exponentiation(tmp);
  return result == libff::alt_bn128_GT::one();
}