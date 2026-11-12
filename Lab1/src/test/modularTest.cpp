#include "../modular.hpp"
#include <iostream>
#include <cassert>

using namespace std;

void test_mod() {
  cout << "[TEST] Mod(a, m)... ";

  // a < m
  uint512_t a = 42ULL;
  uint512_t m = 100ULL;
  assert(Mod(a, m) == a);

  // a == m
  assert(Mod(m, m).is_zero());

  // a > m
  uint512_t x = 123456789ULL;
  uint512_t mod1000 = 1000ULL;
  assert(Mod(x, mod1000) == uint512_t(789ULL));

  // Multi-limb number mod m
  // (1 << 64) % 1000000007
  // 2^64 = 18446744073709551616
  // 18446744073709551616 % 1000000007 = 582344008
  uint512_t two_pow_64 = uint512_t(1ULL) << 64;
  uint512_t prime_m = 1000000007ULL;
  assert(Mod(two_pow_64, prime_m) == uint512_t(582344008ULL));

  cout << "PASSED\n";
}

void test_add_mod() {
  cout << "[TEST] AddMod(x, y, m)... ";

  uint512_t m = 1000ULL;
  uint512_t x = 300ULL;
  uint512_t y = 400ULL;
  assert(AddMod(x, y, m) == uint512_t(700ULL));

  // Sum exceeds m
  uint512_t z = 800ULL;
  assert(AddMod(x, z, m) == uint512_t(100ULL)); // (300 + 800) % 1000 = 100

  // Edge cases
  assert(AddMod(uint512_t(999ULL), uint512_t(1ULL), m).is_zero());
  assert(AddMod(uint512_t(0ULL), uint512_t(0ULL), m).is_zero());

  cout << "PASSED\n";
}

void test_mul_mod() {
  cout << "[TEST] MulMod(x, y, m)... ";

  uint512_t m = 1000000007ULL;
  uint512_t x = 123456789ULL;
  uint512_t y = 987654321ULL;

  // (123456789 * 987654321) % 1000000007 = 259106859
  assert(MulMod(x, y, m) == uint512_t(259106859ULL));

  // Multiply by 0
  assert(MulMod(x, uint512_t(0ULL), m).is_zero());

  // Multiply by 1
  assert(MulMod(x, uint512_t(1ULL), m) == Mod(x, m));

  cout << "PASSED\n";
}

void test_exp_mod() {
  cout << "[TEST] ExpMod(base, exp, m)... ";

  uint512_t m = 1000000007ULL;

  // 2^10 % 1000 = 1024 % 1000 = 24
  assert(ExpMod(uint512_t(2ULL), uint512_t(10ULL), uint512_t(1000ULL)) == uint512_t(24ULL));

  // base^0 % m = 1
  assert(ExpMod(uint512_t(12345ULL), uint512_t(0ULL), m) == uint512_t(1ULL));

  // base^1 % m = base % m
  assert(ExpMod(uint512_t(12345ULL), uint512_t(1ULL), m) == uint512_t(12345ULL));

  // Fermat's Little Theorem: a^(p-1) == 1 (mod p) for prime p
  uint512_t p = 1000000007ULL;
  uint512_t a = 98765ULL;
  uint512_t p_minus_1 = p - uint512_t(1ULL);
  assert(ExpMod(a, p_minus_1, p) == uint512_t(1ULL));

  cout << "PASSED\n";
}

void test_diffie_hellman() {
  cout << "[TEST] Diffie-Hellman Key Exchange Simulation... ";

  // Shared parameters (Prime p, Generator g)
  uint512_t p = 1000000007ULL;
  uint512_t g = 5ULL;

  // Alice chooses private key a
  uint512_t a = 123456ULL;
  // Alice computes public key A = g^a mod p
  uint512_t A = ExpMod(g, a, p);

  // Bob chooses private key b
  uint512_t b = 654321ULL;
  // Bob computes public key B = g^b mod p
  uint512_t B = ExpMod(g, b, p);

  // Exchange and compute shared secrets
  // Alice computes s = B^a mod p
  uint512_t s_alice = ExpMod(B, a, p);

  // Bob computes s = A^b mod p
  uint512_t s_bob = ExpMod(A, b, p);

  // Verify both shared secrets match
  assert(s_alice == s_bob);
  assert(!s_alice.is_zero());

  cout << "PASSED (Shared Secret: " << s_alice << ")\n";
}

int main(void) {
  cout << "========================================" << endl;
  cout << "       RUNNING ALL MODULAR TESTS        " << endl;
  cout << "========================================" << endl;

  test_mod();
  test_add_mod();
  test_mul_mod();
  test_exp_mod();
  test_diffie_hellman();

  cout << "========================================" << endl;
  cout << "       ALL MODULAR TESTS PASSED!        " << endl;
  cout << "========================================" << endl;
  return 0;
}
