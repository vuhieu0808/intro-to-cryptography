#ifndef PRIME_HPP
#define PRIME_HPP

#include "../bigint.hpp"
#include "../modular.hpp"
#include "../lib/prng.h"

const uint64_t SMALL_PRIMES[] = {
  2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
  73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151,
  157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233,
  239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313
};
const size_t NUM_SMALL_PRIMES = sizeof(SMALL_PRIMES) / sizeof(SMALL_PRIMES[0]);

template<size_t N>
int isPrimeForSmallPrimes(const BigInt<N> &n) {
  for (size_t i = 0; i < NUM_SMALL_PRIMES; ++i) {
    if (n == BigInt<N>(SMALL_PRIMES[i])) {
      return 1; // n is one of the small primes
    }
    BigInt<N> tmp = n;
    if (tmp.div_small(SMALL_PRIMES[i]) == 0) {
      return -1; // n is divisible by a small prime => not prime
    }
  }
  return 0; // n is not one of the small primes and not divisible by any of them
}

template<size_t N>
bool millerRabinTest(BigInt<N> n, BigInt<N> a, BigInt<N> d, size_t s) {
  // n-1 = 2^s * d
  // a^d mod n
  BigInt<N> x = ExpMod(a, d, n);
  BigInt<N> one(1), n_minus_one = n - one;
  if (x == one || x == n_minus_one) {
    return true; // probably prime
  }
  for (size_t r = 1; r < s; ++r) {
    x = MulMod(x, x, n); // x = x^2 mod n
    if (x == n_minus_one) {
      return true; // -1 => 1
    }
    if (x == one) {
      return false; // ?? => 1 => composite
    }
  }
  return false; // composite
}

template<size_t N>
bool isPrime(BigInt<N> n, prng_ctx *ctx = nullptr, bool primeSmallCheck = true, size_t iterations = 20) {
  BigInt<N> one(1), two(2), three(3);
  if (n < two) {
    return false; // 0 and 1 are not prime
  }
  if (n == two || n == three) {
    return true; // 2 and 3 are prime
  }
  if (n.get_bit(0) == 0) {
    return false; // even numbers greater than 2 are not prime
  }

  if (primeSmallCheck) {
    int small_prime_result = isPrimeForSmallPrimes<N>(n);
    if (small_prime_result == 1) {
      return true;
    } else if (small_prime_result == -1) {
      return false;
    }
  }

  // if n is prime => a^(n-1) = 1 (mod n) for gcd(a, n) = 1
  // decompose n - 1 = 2^s * d

  BigInt<N> d = n - one;
  size_t s = 0;
  for (size_t i = 0; i < N * BigInt<N>::BASE; ++i) {
    if (d.get_bit(i)) {
      s = i;
      break;
    }
  }
  d >>= s;

  for (size_t i = 0; i < iterations; ++i) {
    BigInt<N> a = randomBigIntInRange<N>(ctx, two, n - two);
    if (!millerRabinTest<N>(n, a, d, s)) {
      return false; // composite
    }
  }
  return true; // probably prime
}

#endif PRIME_HPP