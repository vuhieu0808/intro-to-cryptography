#ifndef DH_HPP
#define DH_HPP

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
    BigInt<N> p = q + q + BigInt<N>(1); // p = 2q + 1
    
    int small_prime_result_q = isPrimeForSmallPrimes<N>(q);
    if (small_prime_result_q == -1) continue; // q is composite
    int small_prime_result_p = isPrimeForSmallPrimes<N>(p);
    if (small_prime_result_p == -1) continue; // p is composite
    
    if (small_prime_result_q == 0 && !isPrime(q, ctx, false)) continue;
    if (small_prime_result_p == 0 && !isPrime(p, ctx, false)) continue;

    return p;
  }
}

template<size_t N>
BigInt<N> generate_private_key(prng_ctx *ctx, const BigInt<N> &p) {
  BigInt<N> two(2);
  BigInt<N> p_minus_two = p - two; // p - 2
  return randomBigIntInRange<N>(ctx, two, p_minus_two);
}

// a generator g => g^2 mod p != 1, g^(p-1)/2 mod p != 1
template<size_t N>
BigInt<N> generate_generator(prng_ctx *ctx, const BigInt<N> &p) {
  BigInt<N> one(1), two(2);
  BigInt<N> p_minus_two = p - two; // p - 2
  BigInt<N> g;

  while (true) {
    g = randomBigIntInRange<N>(ctx, two, p_minus_two);
    if (modular_exponentiation(g, two, p) != one && modular_exponentiation(g, (p - one) >> 1, p) != one) {
      return g;
    }
  }
}

#endif // DH_HPP
