#include <bits/stdc++.h>

// Giả lập cấu trúc BigInt N-words (mỗi word 64-bit)
template <size_t N>
struct BigInt
{
  uint64_t digits[N];

  BigInt()
  {
    for (size_t i = 0; i < N; ++i)
      digits[i] = 0;
  }
  BigInt(uint64_t v)
  {
    for (size_t i = 0; i < N; ++i)
      digits[i] = 0;
    digits[0] = v;
  }

  bool operator>=(const BigInt &rhs) const
  {
    for (size_t i = N; i-- > 0;)
    {
      if (digits[i] != rhs.digits[i])
        return digits[i] > rhs.digits[i];
    }
    return true;
  }

  BigInt &operator-=(const BigInt &rhs)
  {
    __int128_t borrow = 0;
    for (size_t i = 0; i < N; ++i)
    {
      __int128_t diff = static_cast<__int128_t>(digits[i]) - rhs.digits[i] - borrow;
      if (diff < 0)
      {
        borrow = 1;
        diff += (static_cast<__int128_t>(1) << 64);
      }
      else
      {
        borrow = 0;
      }
      digits[i] = static_cast<uint64_t>(diff);
    }
    return *this;
  }
};

using uint512_t = BigInt<8>;   // 8 words * 64 bits = 512 bits
using uint1024_t = BigInt<16>; // 16 words * 64 bits = 1024 bits

// ============================================================================
// PHƯƠNG PHÁP A: NHÂN WORD-LEVEL TRƯỜNG HỌC (SCHOOLBOOK MULTIPLICATION)
// 512-bit x 512-bit -> 1024-bit chỉ với đúng 8 x 8 = 64 phép nhân thanh ghi!
// ============================================================================
uint1024_t multiply_schoolbook(const uint512_t &a, const uint512_t &b)
{
  uint1024_t res; // Khởi tạo 16 words toàn số 0

  for (size_t i = 0; i < 8; ++i)
  {
    uint64_t carry = 0;
    for (size_t j = 0; j < 8; ++j)
    {
      // __uint128_t nhân 2 số 64-bit phần cứng chỉ mất 1 chu kỳ CPU
      __uint128_t cur = static_cast<__uint128_t>(a.digits[i]) * b.digits[j] + res.digits[i + j] + carry;

      res.digits[i + j] = static_cast<uint64_t>(cur); // 64-bit thấp
      carry = static_cast<uint64_t>(cur >> 64);       // 64-bit cao truyền sang word tiếp
    }
    res.digits[i + 8] = carry;
  }
  return res;
}

// ============================================================================
// PHƯƠNG PHÁP B: MONTGOMERY MULTIPLICATION (THUẬT TOÁN CIOS)
// Nhân và rút gọn Modulo trong cùng một lượt, KHÔNG CẦN PHÉP CHIA
// ============================================================================

// 1. Tính m' = -m^(-1) mod 2^64 bằng thuật toán Newton-Raphson (chỉ 6 bước lặp)
uint64_t compute_m_prime(uint64_t m0)
{
  uint64_t inv = 1;
  for (int i = 0; i < 6; ++i)
  { // 2^6 = 64 bits
    inv *= 2 - m0 * inv;
  }
  return -inv; // -m^-1 mod 2^64
}

// 2. Nhân Montgomery: (A * B * R^-1) mod m với R = 2^512
// Thuật toán CIOS: Coarsely Integrated Operand Scanning
uint512_t montgomery_mul(const uint512_t &a, const uint512_t &b, const uint512_t &m, uint64_t m_prime)
{
  uint64_t t[10] = {0}; // Mảng tạm N + 2 words (8 + 2 = 10)

  for (size_t i = 0; i < 8; ++i)
  {
    // Bước 1: Nhân a[i] * b và cộng dồn vào t
    __uint128_t carry = 0;
    for (size_t j = 0; j < 8; ++j)
    {
      __uint128_t sum = static_cast<__uint128_t>(a.digits[i]) * b.digits[j] + t[j] + carry;
      t[j] = static_cast<uint64_t>(sum);
      carry = sum >> 64;
    }
    __uint128_t sum = static_cast<__uint128_t>(t[8]) + carry;
    t[8] = static_cast<uint64_t>(sum);
    t[9] = sum >> 64;

    // Bước 2: Khử word thấp bằng cách nhân với m' (Montgomery reduction)
    uint64_t q = t[0] * m_prime; // Chọn q sao cho (t[0] + q*m0) chia hết cho 2^64

    // Cộng q * m vào t và dịch phải 1 word (chia 2^64 mà không tốn công chia)
    __uint128_t sum_q = static_cast<__uint128_t>(q) * m.digits[0] + t[0];
    carry = sum_q >> 64; // t[0] trở thành 0 nên ta dịch phải luôn

    for (size_t j = 1; j < 8; ++j)
    {
      __uint128_t s = static_cast<__uint128_t>(q) * m.digits[j] + t[j] + carry;
      t[j - 1] = static_cast<uint64_t>(s);
      carry = s >> 64;
    }
    sum = static_cast<__uint128_t>(t[8]) + carry;
    t[7] = static_cast<uint64_t>(sum);
    t[8] = t[9] + (sum >> 64);
    t[9] = 0;
  }

  uint512_t res;
  for (size_t i = 0; i < 8; ++i)
    res.digits[i] = t[i];

  // Nếu res >= m thì trừ đi m 1 lần cuối
  if (t[8] > 0 || res >= m)
  {
    res -= m;
  }
  return res;
}

// ============================================================================
// HÀM TEST ĐO TỐC ĐỘ (BENCHMARK)
// ============================================================================
int main()
{
  uint512_t a, b, m;
  // Khởi tạo các số ngẫu nhiên 512-bit
  for (int i = 0; i < 8; ++i)
  {
    a.digits[i] = 0xA1B2C3D4E5F60718ULL + i;
    b.digits[i] = 0x123456789ABCDEF0ULL + i;
    m.digits[i] = 0xFEDCBA9876543210ULL + i;
  }
  m.digits[0] |= 1ULL; // Đảm bảo m là số lẻ

  const int NUM_RUNS = 100000; // 100.000 phép tính

  std::cout << "--- BENCHMARK 100.000 PHEP NHAN 512-BIT ---\n";

  // 1. Đo tốc độ Schoolbook
  auto t1 = std::chrono::high_resolution_clock::now();
  uint1024_t dummy_schoolbook;
  for (int k = 0; k < NUM_RUNS; ++k)
  {
    dummy_schoolbook = multiply_schoolbook(a, b);
  }
  auto t2 = std::chrono::high_resolution_clock::now();
  std::cout << "1. Schoolbook 8x8 (1024-bit): "
            << std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms\n";

  // 2. Đo tốc độ Montgomery Multiplication
  uint64_t m_prime = compute_m_prime(m.digits[0]);
  auto t3 = std::chrono::high_resolution_clock::now();
  uint512_t dummy_mont;
  for (int k = 0; k < NUM_RUNS; ++k)
  {
    dummy_mont = montgomery_mul(a, b, m, m_prime);
  }
  auto t4 = std::chrono::high_resolution_clock::now();
  std::cout << "2. Montgomery MulMod (512-bit): "
            << std::chrono::duration<double, std::milli>(t4 - t3).count() << " ms\n";

  std::cout << "\n=> Trung binh 1 phep nhan Montgomery chi ton khoang: "
            << (std::chrono::duration<double, std::nano>(t4 - t3).count() / NUM_RUNS)
            << " nanoseconds!\n";

  return 0;
}