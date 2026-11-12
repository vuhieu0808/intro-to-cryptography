#ifndef MODULAR_HPP
#define MODULAR_HPP

#include "bigint.hpp"

// assum x < m, y < m
template<size_t N>
BigInt<N> addReduced(const BigInt<N> &x, const BigInt<N> &y, const BigInt<N> &m) { 
  if (x >= m - y) {
    return x - (m - y);
  }
  return x + y;
}

template<size_t N>
BigInt<N> Mod(const BigInt<N> &a, const BigInt<N> &m) {
  BigInt<N> rem = 0;
  for (size_t i = N * BigInt<N>::BASE; i-- > 0; ) { 
    rem = addReduced(rem, rem, m);
    if (a.get_bit(i)) {
      rem = addReduced(rem, BigInt<N>(1), m);
    }
  }
  return rem;
}

template<size_t N>
BigInt<N> AddMod(const BigInt<N> &x, const BigInt<N> &y, const BigInt<N> &m) {
  BigInt<N> x_mod = Mod(x, m);
  BigInt<N> y_mod = Mod(y, m);
  return addReduced(x_mod, y_mod, m);
}

template<size_t N>
BigInt<N> MulMod(const BigInt<N> &x, const BigInt<N> &y, const BigInt<N> &m) {
  BigInt<N> x_mod = Mod(x, m);
  BigInt<N> res = 0;
  for (size_t i = 0; i < N * BigInt<N>::BASE; ++i) {
    if (y.get_bit(i)) {
      res = addReduced(res, x_mod, m);
    }
    x_mod = addReduced(x_mod, x_mod, m);
  }
  return res;
}

template<size_t N>
BigInt<N> ExpMod(const BigInt<N> &x, const BigInt<N> &y, const BigInt<N> &m) {
  if (y.is_zero()) {
    return Mod(BigInt<N>(1), m);
  }
  BigInt<N> base = Mod(x, m);
  BigInt<N> res = 1;
  size_t highest_bit = 0;
  for (size_t i = N * BigInt<N>::BASE; i-- > 0; ) {
    if (y.get_bit(i)) {
      highest_bit = i;
      break;
    }
  }
  for (size_t i = 0; i <= highest_bit; ++i) {
    if (y.get_bit(i)) {
      res = MulMod(res, base, m);
    }
    base = MulMod(base, base, m);
  }
  return res;
}

#endif // MODULAR_HPP 