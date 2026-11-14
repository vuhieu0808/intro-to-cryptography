#ifndef DH_H
#define DH_H

#include <iostream>
#include "lib/prng.h"
#include "utils/prime.hpp"
#include "bigint.hpp"

template<size_t N>
BigInt<N> modular_exponentiation(const BigInt<N> &base, const BigInt<N> &exp, const BigInt<N> &mod) {
  BigInt<N> result(1);
  BigInt<N> b = Mod(base, mod);
  BigInt<N> e = exp;

  while (!e.is_zero()) {
      if (e.digits[0] & 1) {
        result = MulMod(result, b, mod);
      }
      e >>= 1;
      b = MulMod(b, b, mod);
  }

  return result;
}

template<size_t N>
BigInt<N> generate_safe_prime(prng_ctx *ctx, size_t bitsize = 512) {
  // p = 2q + 1, where q is prime and p is prime => p is a safe prime
  while (true) {
    BigInt<N> q = randomBigIntWithBitLength<N>(ctx, bitsize - 1);
    q.digits[0] |= 1ULL; // Ensure q is odd
    if (!isPrime(q, ctx)) continue;

    BigInt<N> p = q + q + BigInt<N>(1); // p = 2q + 1
    if (isPrime(p, ctx)) {
      std::cout << "Generated safe prime p: " << p << '\n';
      return p;
    }
  }
}


template<size_t N>
BigInt<N> generate_private_key(prng_ctx *ctx, const BigInt<N> &p);

#endif
