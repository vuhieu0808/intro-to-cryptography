#include "../dh.hpp"
#include "../lib/prng.h"
#include <iostream>
#include <chrono>

int main() {
  // 1. Khởi tạo bộ sinh ngẫu nhiên PRNG
  prng_ctx ctx;
  uint8_t seed[32];
  randombytes(seed, sizeof(seed));
  prng_seed(&ctx, seed, sizeof(seed));

  std::cout << "--- TEST 1: Sinh Safe Prime 64-bit ---\n";
  auto start = std::chrono::high_resolution_clock::now();
  uint512_t p64 = generate_safe_prime<8>(&ctx, 64);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;

  uint512_t q64 = (p64 - uint512_t(1)) >> 1;
  std::cout << "p (64-bit): " << p64.to_dec_string() << "\n";
  std::cout << "q = (p-1)/2: " << q64.to_dec_string() << "\n";
  std::cout << "isPrime(p): " << (isPrime(p64, &ctx) ? "TRUE" : "FALSE") << "\n";
  std::cout << "isPrime(q): " << (isPrime(q64, &ctx) ? "TRUE" : "FALSE") << "\n";
  std::cout << "Thoi gian: " << elapsed.count() << " giay\n\n";

  std::cout << "--- TEST 2: Sinh Safe Prime 512-bit ---\n";
  std::cout << "Dang sinh so nguyen to 512-bit, vui long doi...\n";
  start = std::chrono::high_resolution_clock::now();
  uint512_t p512 = generate_safe_prime<8>(&ctx, 512);
  end = std::chrono::high_resolution_clock::now();
  elapsed = end - start;

  uint512_t q512 = (p512 - uint512_t(1)) >> 1;
  std::cout << "p (512-bit): " << p512.to_dec_string() << "\n";
  std::cout << "q = (p-1)/2: " << q512.to_dec_string() << "\n";
  std::cout << "isPrime(p): " << (isPrime(p512, &ctx) ? "TRUE" : "FALSE") << "\n";
  std::cout << "isPrime(q): " << (isPrime(q512, &ctx) ? "TRUE" : "FALSE") << "\n";
  std::cout << "Thoi gian: " << elapsed.count() << " giay\n";

  return 0;
}