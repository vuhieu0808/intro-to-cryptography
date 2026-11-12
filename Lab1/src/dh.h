#ifndef DH_H
#define DH_H

#include <stdint.h>
#include "lib/prng.h"
#include "bigint.hpp"

BigInt modular_exponentiation(const BigInt &base, const BigInt &exp, const BigInt &mod);
BigInt generate_safe_prime(prng_ctx *ctx, size_t bitsize = 512);
BigInt generate_private_key(prng_ctx *ctx, const BigInt &p);

#endif
