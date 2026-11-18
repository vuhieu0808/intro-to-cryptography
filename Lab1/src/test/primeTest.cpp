#include <iostream>
#include <cassert>
#include <vector>
#include "../utils/prime.hpp"
#include "../modular.hpp"
#include "../bigint.hpp"
#include "../lib/prng.h"

using namespace std;

// Kiểm tra Fermat: a^(n-1) == 1 (mod n)
// Dùng để minh họa số Carmichael đánh lừa kiểm tra Fermat như thế nào
template<size_t N>
bool fermatPrimalityTest(const BigInt<N> &n, const BigInt<N> &a) {
  BigInt<N> one(1);
  BigInt<N> n_minus_1 = n - one;
  return ExpMod(a, n_minus_1, n) == one;
}

// 1. Kiểm thử số nhỏ (Small Primes & Small Composites)
void test_small_numbers(prng_ctx *ctx) {
  cout << "[TEST 1] Kiem tra so nho (Small Numbers)...\n";

  // Biên đặc biệt
  assert(!isPrime(uint512_t(0ULL), ctx));
  assert(!isPrime(uint512_t(1ULL), ctx));
  assert(isPrime(uint512_t(2ULL), ctx));
  assert(isPrime(uint512_t(3ULL), ctx));

  // Hợp số chẵn
  vector<uint64_t> even_composites = {4, 6, 8, 10, 100, 256, 1024};
  for (uint64_t c : even_composites) {
    assert(!isPrime(uint512_t(c), ctx));
  }

  // Số nguyên tố nhỏ lẻ
  vector<uint64_t> small_primes = {
    5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47,
    53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 541, 997
  };
  for (uint64_t p : small_primes) {
    // Test cả 2 chế độ: lọc số nguyên tố nhỏ và thuần Miller-Rabin
    assert(isPrime(uint512_t(p), ctx, true));
    assert(isPrime(uint512_t(p), ctx, false));
  }

  // Hợp số nhỏ lẻ (lọt qua bước kiểm tra số chẵn)
  vector<uint64_t> odd_composites = {
    9, 15, 21, 25, 27, 33, 35, 49, 77, 91, 121, 143, 169, 221, 289, 323, 899, 961
  };
  for (uint64_t c : odd_composites) {
    assert(!isPrime(uint512_t(c), ctx, true));
    // Ép chạy Miller-Rabin thuần túy (không dùng trial division số nhỏ)
    assert(!isPrime(uint512_t(c), ctx, false));
  }

  cout << "  -> PASSED: Tat ca cac so nho, nguyen to va hop so deu chinh xac.\n\n";
}

// 2. Kiểm thử số Carmichael (Fermat Pseudoprimes)
void test_carmichael_numbers(prng_ctx *ctx) {
  cout << "[TEST 2] Kiem tra so Carmichael (Fermat Pseudoprimes)...\n";

  // Danh sách các số Carmichael nổi tiếng:
  // 561   = 3 * 11 * 17 (Số Carmichael nhỏ nhất)
  // 1105  = 5 * 13 * 17
  // 1729  = 7 * 13 * 19 (Số Hardy-Ramanujan)
  // 2465  = 5 * 17 * 29
  // 2821  = 7 * 13 * 31
  // 6601  = 7 * 23 * 41
  // 8911  = 7 * 19 * 67
  // 41041 = 7 * 11 * 13 * 41
  vector<uint64_t> carmichaels = {561, 1105, 1729, 2465, 2821, 6601, 8911, 41041};

  // Đối chiếu giữa phép thử Fermat và Miller-Rabin
  for (uint64_t c : carmichaels) {
    uint512_t num(c);

    // Fermat test với cơ số 2 (gcd(2, c) = 1 vì tất cả số Carmichael đều là số lẻ và không chia hết cho 2)
    bool fermat_passed = fermatPrimalityTest(num, uint512_t(2ULL));
    
    // Miller-Rabin TẮT bộ lọc số nhỏ (primeSmallCheck = false) để kiểm tra năng lực thực sự của Miller-Rabin
    bool mr_pure_result = isPrime(num, ctx, false, 20);

    // Miller-Rabin BẬT bộ lọc số nhỏ
    bool mr_filter_result = isPrime(num, ctx, true, 20);

    cout << "  - So Carmichael " << c << ":\n";
    cout << "    + Fermat Test (co so a=2): " 
         << (fermat_passed ? "BỊ ĐÁNH LỪA (Báo Nguyên Tố)" : "Phát hiện Hợp Số") << "\n";
    cout << "    + Miller-Rabin (khong dung bo loc nho): " 
         << (mr_pure_result ? "Thất bại" : "CHÍNH XÁC (Phát hiện Hợp Số)") << "\n";
    cout << "    + Miller-Rabin (dung bo loc nho): " 
         << (mr_filter_result ? "Thất bại" : "CHÍNH XÁC (Phát hiện Hợp Số)") << "\n";

    // Khẳng định: Fermat bị lừa với cơ số 2
    assert(fermat_passed == true);

    // Khẳng định: Miller-Rabin phát hiện chính xác là hợp số (dù có hay không có bộ lọc số nhỏ)
    assert(mr_pure_result == false);
    assert(mr_filter_result == false);
  }

  cout << "  -> PASSED: Miller-Rabin khac phuc hoan toan diem yeu cua Fermat voi so Carmichael.\n\n";
}

// 3. Kiểm thử số lớn (Large Primes & Large Composites)
void test_large_numbers(prng_ctx *ctx) {
  cout << "[TEST 3] Kiem tra so lon (Large Primes & Semiprimes)...\n";

  // A. Các số nguyên tố lớn đã biết (Known Large Primes)
  cout << "  A. Kiem tra cac so nguyen to lon:\n";
  vector<pair<string, uint512_t>> large_primes = {
    {"10^9 + 7 (30-bit)", uint512_t(1000000007ULL)},
    {"10^9 + 9 (30-bit)", uint512_t(1000000009ULL)},
    {"2^31 - 1 (Mersenne M31)", uint512_t(2147483647ULL)},
    {"4294967291 (Max 32-bit prime)", uint512_t(4294967291ULL)},
    {"2^64 - 59 (Max 64-bit prime)", uint512_t(18446744073709551557ULL)},
    {"10^18 + 31 (60-bit prime)", uint512_t(1000000000000000031ULL)}
  };

  for (auto &item : large_primes) {
    cout << "    * Testing " << item.first << "... ";
    assert(isPrime(item.second, ctx, true, 20) == true);
    assert(isPrime(item.second, ctx, false, 20) == true);
    cout << "PRIME (PASSED)\n";
  }

  // Mersenne Prime M127 = 2^127 - 1 (Số nguyên tố 127-bit nổi tiếng)
  uint512_t m127 = (uint512_t(1ULL) << 127) - uint512_t(1ULL);
  cout << "    * Testing 2^127 - 1 (Mersenne M127, 127-bit)... ";
  assert(isPrime(m127, ctx, false, 20) == true);
  cout << "PRIME (PASSED)\n";

  // B. Các hợp số lớn (Large Composites / Semiprimes)
  // Các hợp số này có thừa số nguyên tố > 10^9, hoàn toàn lọt qua bộ lọc 168 số nguyên tố nhỏ (đến 997),
  // buộc thuật toán Miller-Rabin phải tự phân tích và phát hiện.
  cout << "\n  B. Kiem tra cac hop so lon (Semiprimes voi uoc lon > 10^9):\n";

  // n1 = (10^9 + 7) * (10^9 + 9) = 1000000016000000063 (60-bit semiprime)
  uint512_t semi1(1000000016000000063ULL);
  cout << "    * Testing (10^9 + 7) * (10^9 + 9)... ";
  assert(isPrime(semi1, ctx, true, 20) == false);
  assert(isPrime(semi1, ctx, false, 20) == false);
  cout << "COMPOSITE (PASSED)\n";

  // n2 = (2^31 - 1)^2 = 4611686014132420609 (Bình phương của số nguyên tố Mersenne)
  uint512_t semi2(4611686014132420609ULL);
  cout << "    * Testing (2^31 - 1)^2... ";
  assert(isPrime(semi2, ctx, true, 20) == false);
  assert(isPrime(semi2, ctx, false, 20) == false);
  cout << "COMPOSITE (PASSED)\n";

  // n3 = 2^64 - 1 = 18446744073709551615 (Hợp số 64-bit)
  uint512_t all_ones_64 = ~uint512_t(0ULL);
  all_ones_64 = all_ones_64 & ((uint512_t(1ULL) << 64) - uint512_t(1ULL));
  cout << "    * Testing 2^64 - 1 (64-bit composite)... ";
  assert(isPrime(all_ones_64, ctx, false, 20) == false);
  cout << "COMPOSITE (PASSED)\n";

  // n4 = 2^67 - 1 (Mersenne M67 là hợp số: 193707721 * 761838257287)
  uint512_t m67 = (uint512_t(1ULL) << 67) - uint512_t(1ULL);
  cout << "    * Testing 2^67 - 1 (Mersenne M67 composite)... ";
  assert(isPrime(m67, ctx, false, 20) == false);
  cout << "COMPOSITE (PASSED)\n";

  cout << "  -> PASSED: Miller-Rabin nhan dien dung moi so nguyen to lon va hop so semiprime.\n\n";
}

// 4. Kiểm thử tính chất Safe Prime (p = 2q + 1)
void test_safe_primes(prng_ctx *ctx) {
  cout << "[TEST 4] Kiem tra tinh chat Safe Prime (p = 2q + 1)...\n";

  // Cặp safe primes đã biết: (q, p = 2q + 1)
  vector<pair<uint64_t, uint64_t>> safe_pairs = {
    {5, 11},
    {11, 23},
    {23, 47},
    {29, 59},
    {41, 83},
    {53, 107},
    {89, 179},
    {500000003ULL, 1000000007ULL} // 10^9 + 7 là safe prime với q = 500000003
  };

  for (auto &pair : safe_pairs) {
    uint512_t q(pair.first);
    uint512_t p(pair.second);

    // Xác nhận p = 2q + 1
    assert(p == q + q + uint512_t(1ULL));

    // Xác nhận cả p và q đều là số nguyên tố
    assert(isPrime(q, ctx, true, 20) == true);
    assert(isPrime(p, ctx, true, 20) == true);
    cout << "  * Safe prime pair (q=" << pair.first << ", p=" << pair.second << "): PASSED\n";
  }

  cout << "  -> PASSED: Kiem tra Safe Prime hoan toan chinh xac.\n\n";
}

int main() {
  cout << "========================================================\n";
  cout << "   CHUONG TRINH KIEM THU MILLER-RABIN & SO NGUYEN TO   \n";
  cout << "========================================================\n\n";

  // Khởi tạo PRNG
  prng_ctx ctx;
  uint8_t seed[32];
  randombytes(seed, sizeof(seed));
  prng_seed(&ctx, seed, sizeof(seed));

  test_small_numbers(&ctx);
  test_carmichael_numbers(&ctx);
  test_large_numbers(&ctx);
  test_safe_primes(&ctx);

  cout << "========================================================\n";
  cout << ">>> TAT CA CAC BAI KIEM THU MILLER-RABIN DEU PASSED! <<<\n";
  cout << "========================================================\n";

  return 0;
}
