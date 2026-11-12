#include "../bigint.hpp"
#include <iostream>
#include <cassert>
#include <string>

using namespace std;

void test_addition() {
  cout << "[TEST] Addition (+, +=)... ";
  
  // Test basic addition without carry
  uint512_t a = 100ULL;
  uint512_t b = 200ULL;
  assert((a + b) == uint512_t(300ULL));

  // Test carry into next 64-bit limb
  uint512_t max64;
  max64.digits[0] = ~0ULL; // 2^64 - 1
  uint512_t one = 1ULL;
  uint512_t c = max64 + one;
  assert(c.digits[0] == 0ULL);
  assert(c.digits[1] == 1ULL);
  for (size_t i = 2; i < 8; ++i) assert(c.digits[i] == 0ULL);

  // Test user's case (decimal display test)
  uint512_t u1 = 12345678901234567890ULL;
  uint512_t u2 = 8765432109876543210ULL;
  uint512_t sum = u1 + u2;
  assert(sum.to_dec_string() == "21111111011111111100");

  // Test cascading carry through all limbs (overflow wrap-around)
  uint512_t all_ones;
  for (size_t i = 0; i < 8; ++i) all_ones.digits[i] = ~0ULL;
  uint512_t wrapped = all_ones + one;
  assert(wrapped.is_zero());

  // Test self addition (aliasing a += a)
  uint512_t x = 500ULL;
  x += x;
  assert(x == uint512_t(1000ULL));

  cout << "PASSED\n";
}

void test_subtraction() {
  cout << "[TEST] Subtraction (-, -=)... ";

  uint512_t a = 500ULL;
  uint512_t b = 200ULL;
  assert((a - b) == uint512_t(300ULL));

  // Test borrow from higher limb
  uint512_t higher;
  higher.digits[1] = 1ULL; // 2^64
  uint512_t one = 1ULL;
  uint512_t diff = higher - one;
  assert(diff.digits[1] == 0ULL);
  assert(diff.digits[0] == ~0ULL); // 2^64 - 1

  // Test self subtraction
  uint512_t s = 123456789ULL;
  s -= s;
  assert(s.is_zero());

  cout << "PASSED\n";
}

void test_comparison() {
  cout << "[TEST] Comparisons (==, !=, <, >, <=, >=)... ";

  uint512_t a = 100ULL;
  uint512_t b = 200ULL;
  uint512_t c = 100ULL;

  assert(a == c);
  assert(a != b);
  assert(a < b);
  assert(b > a);
  assert(a <= c);
  assert(a <= b);
  assert(a >= c);
  assert(b >= a);

  // Compare across different limbs
  uint512_t big_limb, small_limb;
  big_limb.digits[1] = 1ULL;
  small_limb.digits[0] = ~0ULL;
  assert(big_limb > small_limb);
  assert(small_limb < big_limb);

  cout << "PASSED\n";
}

void test_bitwise() {
  cout << "[TEST] Bitwise (&, |, ^, ~)... ";

  uint512_t a = 0b1010ULL;
  uint512_t b = 0b1100ULL;

  assert((a & b) == uint512_t(0b1000ULL));
  assert((a | b) == uint512_t(0b1110ULL));
  assert((a ^ b) == uint512_t(0b0110ULL));

  // Bitwise NOT
  uint512_t zero;
  uint512_t not_zero = ~zero;
  for (size_t i = 0; i < 8; ++i) {
    assert(not_zero.digits[i] == ~0ULL);
  }

  cout << "PASSED\n";
}

void test_shifts() {
  cout << "[TEST] Bit shifts (<<, >>)... ";

  uint512_t one = 1ULL;

  // Shift within 1 limb
  uint512_t s4 = one << 4;
  assert(s4 == uint512_t(16ULL));

  // Shift across limb boundary (e.g., 64 bits)
  uint512_t s64 = one << 64;
  assert(s64.digits[0] == 0ULL);
  assert(s64.digits[1] == 1ULL);

  // Shift back
  uint512_t back = s64 >> 64;
  assert(back == one);

  // Shift across non-multiple of 64
  uint512_t s70 = one << 70;
  assert(s70.digits[0] == 0ULL);
  assert(s70.digits[1] == (1ULL << 6));
  assert((s70 >> 70) == one);

  // Shift >= total bits
  uint512_t s_huge = one << 512;
  assert(s_huge.is_zero());
  uint512_t r_huge = s64 >> 512;
  assert(r_huge.is_zero());

  cout << "PASSED\n";
}

int main(void) {
  cout << "========================================" << endl;
  cout << "       RUNNING ALL BIGINT TESTS         " << endl;
  cout << "========================================" << endl;

  test_addition();
  test_subtraction();
  test_comparison();
  test_bitwise();
  test_shifts();

  cout << "========================================" << endl;
  cout << "       ALL BIGINT TESTS PASSED!         " << endl;
  cout << "========================================" << endl;
  return 0;
}
