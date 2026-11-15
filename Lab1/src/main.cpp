#include <iostream>
#include <cstring>
#include <chrono>
#include "bigint.hpp"
#include "dh.hpp"
#include "lib/prng.h"

int main(void) {
  uint8_t seed[32];
  randombytes(seed, sizeof(seed)); // Generate a random seed for the PRNG

  prng_ctx ctx;
  prng_seed(&ctx, seed, sizeof(seed));

  int bitSize = 512; // 8 bytes * 64 bits = 512 bits

  auto start = std::chrono::high_resolution_clock::now();

  // Generate a safe prime p
  uint512_t p = generate_safe_prime<8>(&ctx, bitSize);
  // Generate a generator g
  uint512_t g = generate_generator<8>(&ctx, p);

  // private key of Alice & Bob
  uint512_t a = generate_private_key<8>(&ctx, p);
  uint512_t b = generate_private_key<8>(&ctx, p);

  // public key of Alice & Bob
  uint512_t A = modular_exponentiation<8>(g, a, p); // A = g^a mod p
  uint512_t B = modular_exponentiation<8>(g, b, p); // B = g^b mod p

  // shared secret key of Alice & Bob
  uint512_t AliceSharedKey = modular_exponentiation<8>(B, a, p); // B^a mod p = (g^b)^a mod p = g^(ab) mod p
  uint512_t BobSharedKey = modular_exponentiation<8>(A, b, p); // A^b mod p = (g^a)^b mod p = g^(ab) mod p
  std::cout << "Safe prime p: " << p << '\n';
  std::cout << "Generator g: " << g << '\n';
  std::cout << "Alice's private key a: " << a << '\n';
  std::cout << "Bob's private key b: " << b << '\n';
  std::cout << "Alice's public key A: " << A << '\n';
  std::cout << "Bob's public key B: " << B << '\n';
  std::cout << "Alice's shared key: " << AliceSharedKey << '\n';
  std::cout << "Bob's shared key: " << BobSharedKey << '\n';
  std::cout << "Shared keys are equal: " << (AliceSharedKey == BobSharedKey ? "true" : "false") << '\n';

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  std::cout << "Elapsed time: " << elapsed.count() << " seconds\n";

  return 0;
}