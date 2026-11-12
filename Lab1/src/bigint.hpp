#ifndef BIGINT_H
#define BIGINT_H

#include <cstdint>
#include <cstddef>
#include <iostream>
#include <string>
#include <algorithm>

template<size_t N>
struct BigInt {
  const static size_t BASE = 64; // uint64_t = 2^64 ~ 64 bits
  uint64_t digits[N];
  
  BigInt() {
    for (size_t i = 0; i < N; ++i) {
      digits[i] = 0;
    }
  }

  BigInt(uint64_t value) {
    for (size_t i = 0; i < N; ++i) {
      digits[i] = 0;
    }
    digits[0] = value;
  }

  // math operators
  BigInt& operator += (const BigInt &rhs) {
    __uint128_t carry = 0;
    for (size_t i = 0; i < N; ++i) {
      __uint128_t sum = static_cast<__uint128_t>(digits[i]) + rhs.digits[i] + carry;
      digits[i] = static_cast<uint64_t>(sum);
      carry = sum >> BASE;
    }
    return *this;
  }

  BigInt operator + (const BigInt &rhs) const {
    BigInt result = *this;
    result += rhs;
    return result;
  }

  // assume that the result will not be negative
  BigInt& operator -= (const BigInt &rhs) {
    __int128_t borrow = 0;
    for (size_t i = 0; i < N; ++i) {
      __int128_t diff = static_cast<__int128_t>(digits[i]) - rhs.digits[i] - borrow;
      if (diff < 0) {
        borrow = 1;
        diff += (static_cast<__int128_t>(1) << BASE);
      } else {
        borrow = 0;
      }
      digits[i] = static_cast<uint64_t>(diff);
    }
    return *this;
  }

  BigInt operator - (const BigInt &rhs) const {
    BigInt result = *this;
    result -= rhs;
    return result;
  }

  // compare operators

  // x > y => 1, x < y => -1, x == y => 0
  #define COMPARE(x, y) (((x) > (y)) - ((x) < (y)))
  int compare(const BigInt &rhs) const {
    for (size_t i = N; i-- > 0; ) {
      if (digits[i] != rhs.digits[i]) {
        return COMPARE(digits[i], rhs.digits[i]);
      }
    }
    return 0;
  }

  #define DEFINE_COMPARISON_OPERATOR(op) bool operator op (const BigInt &rhs) const { return compare(rhs) op 0; }
    DEFINE_COMPARISON_OPERATOR(==) 
    DEFINE_COMPARISON_OPERATOR(!=)
    DEFINE_COMPARISON_OPERATOR(<) 
    DEFINE_COMPARISON_OPERATOR(>) 
    DEFINE_COMPARISON_OPERATOR(<=)
    DEFINE_COMPARISON_OPERATOR(>=)
  #undef DEFINE_COMPARISON_OPERATOR

  // bitwise operators
  BigInt& operator &= (const BigInt &rhs) {
    for (size_t i = 0; i < N; ++i) {
      digits[i] &= rhs.digits[i];
    }
    return *this;
  }
  BigInt operator & (const BigInt &rhs) const {
    BigInt result = *this;
    result &= rhs;
    return result;
  }

  BigInt& operator |= (const BigInt &rhs) {
    for (size_t i = 0; i < N; ++i) {
      digits[i] |= rhs.digits[i];
    }
    return *this;
  }
  BigInt operator | (const BigInt &rhs) const {
    BigInt result = *this;
    result |= rhs;
    return result;
  }

  BigInt& operator ^= (const BigInt &rhs) {
    for (size_t i = 0; i < N; ++i) {
      digits[i] ^= rhs.digits[i];
    }
    return *this;
  }
  BigInt operator ^ (const BigInt &rhs) const {
    BigInt result = *this;
    result ^= rhs;
    return result;
  }

  BigInt operator ~ () const {
    BigInt result;
    for (size_t i = 0; i < N; ++i) {
      result.digits[i] = ~digits[i];
    }
    return result;
  }

  BigInt& operator <<= (size_t shift) {
    if (shift >= N * BASE) {
      for (size_t i = 0; i < N; ++i) {
        digits[i] = 0;
      }
      return *this;
    }

    size_t whole_shift = shift / BASE;
    size_t bit_shift = shift % BASE;

    for (size_t i = N; i-- > 0; ) {
      uint64_t new_value = 0;
      if (i >= whole_shift) {
        new_value = digits[i - whole_shift] << bit_shift;
        if (bit_shift > 0 && i - whole_shift > 0) {
          new_value |= digits[i - whole_shift - 1] >> (BASE - bit_shift);
        }
      }
      digits[i] = new_value;
    }

    return *this;
  }

  BigInt operator << (size_t shift) const {
    BigInt result = *this;
    result <<= shift;
    return result;
  }

  BigInt& operator >>= (size_t shift) {
    if (shift >= N * BASE) {
      for (size_t i = 0; i < N; ++i) {
        digits[i] = 0;
      }
      return *this;
    }

    size_t whole_shift = shift / BASE;
    size_t bit_shift = shift % BASE;

    for (size_t i = 0; i < N; ++i) {
      uint64_t new_value = 0;
      if (i + whole_shift < N) {
        new_value = digits[i + whole_shift] >> bit_shift;
        if (bit_shift > 0 && i + whole_shift + 1 < N) {
          new_value |= digits[i + whole_shift + 1] << (BASE - bit_shift);
        }
      }
      digits[i] = new_value;
    }

    return *this;
  }

  BigInt operator >> (size_t shift) const {
    BigInt result = *this;
    result >>= shift;
    return result;
  }

  // utility functions
  bool is_zero() const {
    for (size_t i = 0; i < N; ++i) {
      if (digits[i] != 0) return false;
    }
    return true;
  }

  bool get_bit(size_t i) const {
    if (i >= N * BASE) return false;
    return (digits[i / BASE] >> (i % BASE)) & 1ULL;
  }

  uint64_t div_small(uint64_t v) {
    __uint128_t rem = 0;
    for (size_t i = N; i-- > 0; ) {
      __uint128_t cur = (rem << BASE) | digits[i];
      digits[i] = static_cast<uint64_t>(cur / v);
      rem = cur % v;
    }
    return static_cast<uint64_t>(rem);
  }

  std::string to_dec_string() const {
    if (is_zero()) return "0";
    BigInt temp = *this;
    std::string s = "";
    while (!temp.is_zero()) {
      uint64_t rem = temp.div_small(10);
      s += static_cast<char>('0' + rem);
    }
    std::reverse(s.begin(), s.end());
    return s;
  }

  friend std::ostream& operator<<(std::ostream &os, const BigInt &num) {
    os << num.to_dec_string();
    return os;
  }
};

using uint512_t = BigInt<8>;  // 512 bits
using uint1024_t = BigInt<16>; // 1024 bits

#endif // BIGINT_H