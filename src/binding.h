#pragma once

#include <stdbool.h>
#include <stdint.h>

#define BN254_ALIGN (16UL)
#define BN254_G1_FOOTPRINT (64UL)
#define BN254_G2_FOOTPRINT (128UL)
#define BN254_G1_COMPRESSED_FOOTPRINT (32UL)
#define BN254_G2_COMPRESSED_FOOTPRINT (64UL)
#define BN254_BIGINT_FOOTPRINT (32UL)

/* input == [128]u8, out == [64]u8*/
int bn254_add_syscall(uint8_t const *__restrict input, uint8_t *__restrict out);
/* input == [96]u8, out == [64]u8*/
int bn254_mul_syscall(uint8_t const *__restrict input, uint8_t *__restrict out);
/* input_len % 192 == 0, out == [32]u8*/
int bn254_pairing_syscall(uint8_t const *__restrict input, uintptr_t input_len,
                          uint8_t *__restrict out);

/* input == [64]u8, out == [32]u8 */
int bn254_compress_g1_syscall(uint8_t const *__restrict input, uint8_t *__restrict out);
/* input == [32]u8, out == [64]u8 */
int bn254_decompress_g1_syscall(uint8_t const *__restrict input, uint8_t *__restrict out);

/* input == [128]u8, out == [64]u8 */
int bn254_compress_g2_syscall(uint8_t const *__restrict input, uint8_t *__restrict out);
/* input == [64]u8, out == [128]u8 */
int bn254_decompress_g2_syscall(uint8_t const *__restrict input, uint8_t *__restrict out);