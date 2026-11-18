# BÁO CÁO LAB 1: SỐ NGUYÊN TỐ VÀ TRAO ĐỔI KHÓA DIFFIE–HELLMAN

**Môn học:** Nhập môn mã hóa mật mã  
**Ngôn ngữ cài đặt:** C/C++, biên dịch với C++14  
**Tài liệu đề bài:** `Lab1.pdf`  
**Mã nguồn:** thư mục `src/`

| Thành viên | Họ và tên | MSSV |
| --- | --- | --- |
| 1 | [Điền họ và tên] | [Điền MSSV] |
| 2, nếu có | [Điền họ và tên] | [Điền MSSV] |
| 3, nếu có | [Điền họ và tên] | [Điền MSSV] |
| 4, nếu có | [Điền họ và tên] | [Điền MSSV] |

## 1. Mục tiêu và phạm vi bài làm

Bài thực hành xây dựng chương trình mô phỏng trao đổi khóa Diffie–Hellman giữa Alice và Bob. Hai bên sử dụng khóa riêng của mình và giá trị công khai của đối phương để tính ra cùng một bí mật chung, mà không cần truyền trực tiếp bí mật đó.

Theo đề bài, số nguyên tố sử dụng phải có độ lớn tối thiểu 512 bit. Chương trình tự cài đặt kiểu số lớn, phép toán mô-đun và kiểm tra nguyên tố; không sử dụng thư viện số lớn như GMP hoặc Boost.Multiprecision. Phần PRNG và SHAKE sử dụng mã thư viện có trong bộ mã nguồn bài lab.

| Yêu cầu trong đề | Thành phần thực hiện |
| --- | --- |
| Hệ thống xử lý số lớn | `BigInt<N>` trong `bigint.hpp` |
| Lũy thừa mô-đun | `ExpMod` trong `modular.hpp`, được gọi qua `modular_exponentiation` trong `dh.hpp` |
| Kiểm tra nguyên tố | Lọc bằng số nguyên tố nhỏ và Miller–Rabin trong `utils/prime.hpp` |
| Sinh số nguyên tố an toàn | `generate_safe_prime` trong `dh.hpp` |
| Sinh khóa riêng | `generate_private_key` trong `dh.hpp` |
| Chọn phần tử sinh | `generate_generator` trong `dh.hpp` |
| Mô phỏng trao đổi khóa và so sánh kết quả | `main.cpp` |
| Sinh số giả ngẫu nhiên | `lib/prng.cpp`, sử dụng SHAKE128 trong `lib/fips202.c` |

Chương trình hiện mô phỏng cả hai bên trong cùng một tiến trình. Việc trao đổi giá trị công khai được biểu diễn bằng cách sử dụng các biến `A` và `B`; chưa có kết nối mạng giữa hai chương trình độc lập.

## 2. Hệ thống xử lý số lớn

### 2.1. Biểu diễn `BigInt<N>`

Các kiểu nguyên thông thường như `uint64_t` chỉ chứa tối đa 64 bit, không đủ cho số nguyên tố 512 bit. Vì vậy, chương trình biểu diễn một số lớn bằng mảng gồm `N` phần tử, mỗi phần tử là một số nguyên không dấu 64 bit:

```cpp
template<size_t N>
struct BigInt {
    const static size_t BASE = 64;
    uint64_t digits[N];
    // Các hàm và toán tử xử lý số lớn.
};

using uint512_t = BigInt<8>;
using uint1024_t = BigInt<16>;
```

Giá trị số học được xác định bởi:

$$
x = \sum_{i=0}^{N-1} \text{digits}[i] \cdot 2^{64i}.
$$

`digits[0]` chứa 64 bit thấp nhất, còn `digits[N-1]` chứa 64 bit cao nhất. Đây là cách sắp xếp theo thứ tự từ phần có trọng số thấp đến phần có trọng số cao. Ví dụ, số $2^{64}+5$ được lưu với `digits[0] = 5`, `digits[1] = 1`, và các phần còn lại bằng 0.

Miền giá trị của `BigInt<N>` là:

$$
0 \le x \le 2^{64N}-1.
$$

| Kiểu | Số phần tử 64 bit | Số bit | Dung lượng mảng |
| --- | ---: | ---: | ---: |
| `BigInt<8>` / `uint512_t` | 8 | 512 | 64 byte |
| `BigInt<16>` / `uint1024_t` | 16 | 1024 | 128 byte |
| `BigInt<32>` | 32 | 2048 | 256 byte |

`BigInt<32>` có thể được khởi tạo từ mẫu kiểu dữ liệu, nhưng mã hiện tại chưa khai báo bí danh `uint2048_t`.

Hàm tạo mặc định đặt toàn bộ mảng bằng 0. Hàm tạo từ `uint64_t` cũng đặt mảng bằng 0, sau đó gán giá trị đầu vào vào `digits[0]`.

### 2.2. Phép cộng và phép trừ

**Phép cộng:** duyệt từ phần thấp đến phần cao và truyền số nhớ `carry` sang phần tiếp theo:

$$
t_i = x_i+y_i+\text{carry},\qquad
z_i = t_i \bmod 2^{64},\qquad
\text{carry} = \left\lfloor t_i/2^{64}\right\rfloor.
$$

Biến trung gian `__uint128_t` chứa tổng mà không mất bit nhớ. Chẳng hạn, $(2^{64}-1)+1$ tạo ra `digits[0] = 0` và `digits[1] = 1`.

**Phép trừ:** cũng duyệt từ thấp lên cao. Nếu $x_i-y_i-\text{borrow}<0$, chương trình cộng thêm $2^{64}$ vào phần hiện tại và mượn 1 từ phần tiếp theo. Biến trung gian dùng `__int128_t` để nhận biết kết quả âm.

Các toán tử `+` và `-` tạo bản sao rồi gọi `+=` hoặc `-=`. Cách tổ chức này giảm việc lặp lại mã xử lý.

Kiểu dữ liệu có kích thước cố định nên phép cộng bỏ số nhớ vượt khỏi phần tử cuối. Ví dụ, $(2^{512}-1)+1$ trở thành 0 trong `BigInt<8>`. Phép trừ cũng không có kiểu số âm và không báo lỗi khi bị hụt; các thuật toán của bài cần bảo đảm toán hạng trái không nhỏ hơn toán hạng phải.

`__uint128_t` và `__int128_t` là phần mở rộng của trình biên dịch, không phải kiểu được chuẩn C++14 quy định. Chương trình đã được kiểm tra bằng GCC trên Linux; việc chuyển sang trình biên dịch khác cần xem xét khả năng hỗ trợ các kiểu này.

### 2.3. So sánh, thao tác bit và xuất số

| Nhóm thao tác | Cách hoạt động |
| --- | --- |
| `compare`, `==`, `!=`, `<`, `>`, `<=`, `>=` | Duyệt từ phần cao nhất xuống thấp nhất. Phần đầu tiên khác nhau quyết định quan hệ giữa hai số. |
| `&`, `|`, `^`, `~` và các dạng gán | Thực hiện phép toán bit trên từng phần tử 64 bit. `~` đảo toàn bộ `64N` bit. |
| `<<`, `>>`, `<<=`, `>>=` | Tách số bit dịch thành số phần tử nguyên vẹn và số bit dịch bên trong mỗi phần tử, rồi ghép các bit vượt qua ranh giới 64 bit. |
| `is_zero()` | Kiểm tra tất cả phần tử có bằng 0 hay không. |
| `get_bit(i)` | Đọc bit ở `digits[i / 64]`, vị trí `i % 64`; trả về `false` nếu ngoài phạm vi. |
| `div_small(v)` | Chia số lớn cho một số 64 bit, cập nhật đối tượng thành thương và trả về số dư. Điều kiện sử dụng là `v > 0`. |
| `to_dec_string()` | Sao chép số, chia liên tiếp cho 10, thu thập chữ số dư rồi đảo thứ tự. |
| `operator<<` với `ostream` | Xuất chuỗi thập phân từ `to_dec_string()`. |

Khi dịch trái, chương trình duyệt từ cao xuống thấp để không ghi đè dữ liệu còn cần dùng. Khi dịch phải, chương trình duyệt từ thấp lên cao. Nếu số bit dịch lớn hơn hoặc bằng `64N`, kết quả được đặt bằng 0.

Trong `div_small`, tại mỗi bước:

$$
\text{cur}=\text{rem}\cdot 2^{64}+\text{digits}[i].
$$

`cur` dùng 128 bit; phần tử thương bằng `cur / v`, còn số dư mới bằng `cur % v`. Thuật toán đi từ phần cao xuống thấp, tương tự phép chia dài.

Các thao tác cộng, trừ, so sánh và dịch bit có độ phức tạp $O(N)$. Việc xuất thập phân thực hiện nhiều lần chia nhỏ nên tốn chi phí hơn một thao tác trên mảng.

### 2.4. Cộng mô-đun và tránh tràn số

Mục tiêu của phép cộng mô-đun là tính $(x+y)\bmod m$. Nếu cộng trực tiếp hai số gần giới hạn 512 bit, kết quả trung gian có thể cần 513 bit. Khi đó, phép cộng thông thường sẽ mất bit cao và làm sai số dư.

Hàm `addReduced` giải quyết trường hợp này với điều kiện $0\le x,y<m$:

$$
\operatorname{addReduced}(x,y,m)=
\begin{cases}
x-(m-y), & x\ge m-y,\\
x+y, & x<m-y.
\end{cases}
$$

Ở nhánh thứ nhất, kết quả bằng $x+y-m$ nhưng không cần tạo tổng có khả năng tràn. Ở nhánh thứ hai, $x+y<m$ nên phép cộng nằm trong phạm vi biểu diễn. Hai nhánh đều cho kết quả trong $[0,m-1]$.

Hàm `AddMod` trước hết đưa hai toán hạng về số dư bằng `Mod`, sau đó gọi `addReduced`.

### 2.5. Phép lấy dư `Mod`

Hàm `Mod(a,m)` duyệt các bit của `a` từ cao xuống thấp. Nếu số dư hiện tại là `rem` và bit đang đọc là $b_i$, số dư mới là:

$$
\text{rem}_{\text{mới}}=(2\cdot\text{rem}+b_i)\bmod m.
$$

Phép nhân đôi và cộng bit được thực hiện bằng `addReduced` để tránh tràn. Nếu `a < m`, hàm trả về `a` ngay. Thuật toán này không cần cài đặt phép chia giữa hai `BigInt`.

### 2.6. Phép nhân mô-đun `MulMod`

Một tích 512 bit nhân 512 bit có thể cần 1024 bit. Luồng xử lý chính tránh tạo tích đầy đủ bằng phương pháp nhân đôi và cộng theo bit:

1. Đặt `x_mod = x mod m`, `res = 0`.
2. Duyệt các bit của `y` từ thấp lên cao.
3. Nếu bit hiện tại bằng 1, cộng `x_mod` vào `res` theo mô-đun.
4. Nhân đôi `x_mod` theo mô-đun để chuẩn bị cho bit tiếp theo.

Do:

$$
y=\sum_i y_i2^i,
$$

kết quả thu được là:

$$
xy\bmod m=\left(\sum_i y_i(x2^i)\right)\bmod m.
$$

Mã hiện tại duyệt đủ `64N` bit của `y`, ngay cả khi số có nhiều bit cao bằng 0. Ưu điểm là không cần mảng tích 1024 bit; nhược điểm là phải thực hiện nhiều phép cộng và so sánh trên số lớn.

### 2.7. Lũy thừa mô-đun `ExpMod`

Hàm cần tính:

$$
\operatorname{ExpMod}(x,y,m)=x^y\bmod m.
$$

Chương trình sử dụng thuật toán bình phương và nhân. Mỗi bit của số mũ quyết định có nhân cơ số hiện tại vào kết quả hay không; sau mỗi bước, cơ số được bình phương theo mô-đun.

Giả mã sát với cách cài đặt:

```text
ExpMod(x, y, m):
    nếu y = 0:
        trả về Mod(1, m)
    base = Mod(x, m)
    result = 1
    h = vị trí bit 1 cao nhất của y
    với i từ 0 đến h:
        nếu bit i của y bằng 1:
            result = MulMod(result, base, m)
        base = MulMod(base, base, m)
    trả về result
```

Ví dụ, $13=1+4+8$, do đó $x^{13}$ được ghép từ $x$, $x^4$ và $x^8$. Các giá trị trung gian luôn được rút gọn theo mô-đun, thay vì tính toàn bộ $x^{13}$ rồi mới lấy dư.

Nếu số mũ có $t$ bit, thuật toán cần $O(t)$ phép nhân mô-đun, thay cho việc nhân $y$ lần. Đây là số lượng phép nhân mô-đun, không phải toàn bộ chi phí tính toán. Với $W=64N$, `MulMod` có chi phí $O(WN)$ và `ExpMod` có chi phí khoảng $O(tWN)$ theo các vòng lặp hiện tại.

Hàm `modular_exponentiation` trong `dh.hpp` chỉ gọi `ExpMod`, giúp giao diện trùng với nhiệm vụ được nêu trong đề. Các phép toán ở luồng Diffie–Hellman sử dụng mô-đun $m=p>2$. Mã hiện chưa kiểm tra và báo lỗi riêng cho đầu vào `m = 0`.

## 3. Kiểm tra nguyên tố và sinh số nguyên tố an toàn

### 3.1. Các phương pháp kiểm tra nguyên tố

| Phương pháp | Ý tưởng | Nhận xét đối với bài lab |
| --- | --- | --- |
| Chia thử | Kiểm tra các ước đến $\sqrt n$ | Cho kết quả chắc chắn nhưng không khả thi nếu áp dụng đầy đủ cho số 512 bit. |
| Kiểm tra Fermat | Dùng quan hệ $a^{n-1}\equiv1\pmod n$ của số nguyên tố | Một số hợp số vẫn vượt qua, đặc biệt các số Carmichael. |
| Miller–Rabin | Kiểm tra cấu trúc của dãy bình phương sau khi phân tích $n-1=2^sd$ | Hiệu quả với số lớn; được cài đặt trong bài. |
| AKS | Thuật toán tất định có thời gian đa thức | Có ý nghĩa lý thuyết nhưng việc cài đặt và chi phí thực tế không phù hợp với phạm vi bài này. |

Ví dụ, $561=3\cdot11\cdot17$ là hợp số Carmichael: với các cơ số nguyên tố cùng nhau với 561, kiểm tra Fermat vẫn cho kết quả giống số nguyên tố. Vì vậy, chỉ dựa vào định lý Fermat là chưa đủ.

### 3.2. Lọc bằng số nguyên tố nhỏ

`SMALL_PRIMES` chứa 168 số nguyên tố từ 2 đến 997. Hàm `isPrimeForSmallPrimes(n)` trả về:

| Giá trị trả về | Ý nghĩa |
| ---: | --- |
| `1` | `n` bằng một số nguyên tố trong danh sách. |
| `-1` | `n` chia hết cho một số nguyên tố trong danh sách và khác số đó. |
| `0` | Chưa thể kết luận từ danh sách này. |

Để tính số dư, hàm sao chép `n` rồi gọi `div_small`. Bản sao bảo đảm phép chia không thay đổi số đang được kiểm tra.

Lọc trước giúp loại bỏ nhanh nhiều hợp số, giảm số lần gọi lũy thừa mô-đun. Việc vượt qua bộ lọc chỉ cho biết số đó không có ước nguyên tố nhỏ trong danh sách, chưa chứng minh nó là số nguyên tố. Các trường hợp 0 và 1 được hàm `isPrime` xử lý riêng trước khi gọi bộ lọc.

### 3.3. Thuật toán Miller–Rabin

Với số lẻ $n>3$, phân tích:

$$
n-1=2^sd,\qquad d\text{ lẻ}.
$$

Trong mã, `s` được tìm bằng vị trí bit 1 thấp nhất của `n-1`, sau đó `d` được dịch phải `s` bit.

Một vòng kiểm tra thực hiện:

1. Chọn ngẫu nhiên cơ số $a\in[2,n-2]$.
2. Tính $x=a^d\bmod n$ bằng `ExpMod`.
3. Nếu $x=1$ hoặc $x=n-1$, vòng này được chấp nhận.
4. Nếu chưa được chấp nhận, bình phương $x$ theo mô-đun tối đa $s-1$ lần. Nếu gặp $n-1$, vòng này được chấp nhận.
5. Nếu gặp 1 trước khi gặp $n-1$, hoặc hết các lần bình phương mà không gặp $n-1$, kết luận `n` là hợp số.

Đối với số nguyên tố, dãy này phải thỏa các điều kiện trên. Nếu điều kiện không thỏa, cơ số $a$ là một nhân chứng cho tính hợp số. Nếu điều kiện thỏa, chỉ có thể nói `n` vượt qua kiểm tra đối với cơ số đó.

`isPrime` thực hiện xử lý biên, lọc số nhỏ nếu được yêu cầu, rồi lặp Miller–Rabin. Số vòng mặc định là:

```cpp
size_t iterations = 20;
```

Với một hợp số lẻ cố định và các cơ số được lấy độc lập, đều, xác suất vượt qua $k$ vòng có chặn trên:

$$
4^{-k}.
$$

Với 20 vòng, chặn trên là $2^{-40}\approx9.09\times10^{-13}$. Đây là chặn xác suất một hợp số cố định vượt qua kiểm tra; không phải xác suất hậu nghiệm chính xác rằng số được sinh ra là hợp số. Việc sinh số nguyên tố an toàn còn kiểm tra cả `q` và `p`, nên không nên gán nguyên chặn này cho toàn bộ quá trình mà không phân tích thêm.

Kết quả `true` đối với số lớn có nghĩa là **nguyên tố với xác suất cao**, không phải một chứng nhận nguyên tố tất định.

### 3.4. Sinh số nguyên tố an toàn

Một số nguyên tố $p$ được gọi là số nguyên tố an toàn nếu:

$$
p=2q+1
$$

và $q$ cũng là số nguyên tố. Khi đó $q=(p-1)/2$.

Hàm `generate_safe_prime(ctx, bitsize)` thực hiện:

```text
Lặp:
    Sinh q có đúng bitsize - 1 bit
    Đặt bit thấp nhất của q bằng 1 để q lẻ
    Tính p = q + q + 1
    Lọc q và p bằng các số nguyên tố nhỏ
    Nếu một số bị xác định là hợp số: thử ứng viên khác
    Với các số chưa kết luận: chạy Miller–Rabin
    Nếu cả q và p được chấp nhận: trả về p
```

Trong bước Miller–Rabin, hàm gọi `isPrime(..., ctx, false)` vì đã lọc số nhỏ bên ngoài. Điều này tránh lặp lại cùng một bộ lọc.

Với độ dài yêu cầu là $b$, hàm sinh `q` bảo đảm:

$$
2^{b-2}\le q\le2^{b-1}-1.
$$

Suy ra:

$$
2^{b-1}+1\le p=2q+1\le2^b-1,
$$

nên `p` có đúng $b$ bit, với điều kiện kiểu dữ liệu đủ chứa `p`. Khi `b = 512`, `q` có 511 bit và `p` nằm trong phạm vi `BigInt<8>`.

Thuật toán phải tìm đồng thời hai số được chấp nhận là nguyên tố, nên thường tốn thời gian hơn việc sinh một số nguyên tố thông thường. Do ứng viên được chọn ngẫu nhiên, thời gian mỗi lần chạy có thể khác nhau đáng kể.

## 4. Số ngẫu nhiên và vai trò trong mật mã

Phần này trình bày nội dung số ngẫu nhiên, thuộc mục điểm cộng trong đề bài.

### 4.1. Vì sao cần tính ngẫu nhiên?

Trong Diffie–Hellman, nếu đối phương đoán được khóa riêng `a` hoặc `b`, họ có thể tự tính bí mật chung từ giá trị công khai. Vì vậy, khóa riêng cần khó dự đoán, ngay cả khi đối phương biết thuật toán sinh số.

Tính ngẫu nhiên cũng được dùng để chọn ứng viên số nguyên tố, cơ số Miller–Rabin và phần tử sinh `g`. Phân bố đều giúp tránh việc một số giá trị xuất hiện với xác suất cao hơn các giá trị khác do cách ánh xạ đầu ra.

Một chuỗi trông không có quy luật chưa chắc an toàn. Bộ sinh dùng seed là thời gian hiện tại có thể tạo ra chuỗi khó nhìn bằng mắt nhưng vẫn dễ bị đoán hoặc vét cạn seed.

### 4.2. Entropy, PRNG và bộ sinh dùng trong mật mã

- **Entropy:** mức độ bất định của nguồn dữ liệu, liên quan đến khả năng dự đoán giá trị đầu vào.
- **PRNG:** thuật toán xác định mở rộng một seed thành chuỗi số giả ngẫu nhiên. Cùng seed và cùng thứ tự gọi hàm sẽ tạo cùng chuỗi.
- **Bộ sinh phù hợp cho mật mã:** cần có đầu ra khó dự đoán khi chưa biết trạng thái bí mật và được khởi tạo từ nguồn entropy phù hợp.

Một quy trình sinh số trên máy tính là lấy seed từ bộ sinh ngẫu nhiên của hệ điều hành, rồi dùng cơ chế mật mã để tạo các byte cần thiết. Chất lượng seed và việc bảo vệ trạng thái đều ảnh hưởng đến tính an toàn.

### 4.3. Cách cài đặt trong chương trình

`main.cpp` và `test/dhTest.cpp` khởi tạo như sau:

```cpp
uint8_t seed[32];
randombytes(seed, sizeof(seed));

prng_ctx ctx;
prng_seed(&ctx, seed, sizeof(seed));
```

Hàm `randombytes` gọi `getrandom` để lấy byte từ hệ điều hành. Khi bị ngắt bởi tín hiệu (`EINTR`), hàm thử lại; khi gặp lỗi khác, hàm dừng chương trình bằng `abort`. Nếu hệ điều hành trả về ít byte hơn yêu cầu, hàm tiếp tục lấy phần còn thiếu.

Phần cài đặt này phụ thuộc API Linux qua `<sys/random.h>`. Vì vậy, môi trường chạy phù hợp với mã hiện tại là Linux hoặc Ubuntu trong WSL. Bình luận trong mã nhắc đến macOS nhưng lời gọi `getrandom` này không nên được xem là khả chuyển trực tiếp sang macOS hoặc Windows.

`prng_ctx` lưu trạng thái sponge của Keccak. `prng_seed` gọi lần lượt:

```text
shake128_init → shake128_absorb(seed) → shake128_finalize
```

Các hàm `prng_next_u16`, `prng_next_u32` và `prng_next_u64` lấy lần lượt 2, 4 và 8 byte bằng `shake128_squeeze`, rồi ghép thành số nguyên theo thứ tự byte thấp trước. Trạng thái được cập nhật qua các lần gọi để tạo các phần tiếp theo của cùng một luồng đầu ra.

SHAKE128 là hàm đầu ra mở rộng (XOF): từ đầu vào hữu hạn có thể tạo đầu ra với độ dài được yêu cầu. Thư viện sử dụng hoán vị Keccak-f[1600] gồm 24 vòng, với trạng thái 25 phần tử 64 bit. SHAKE128 có tốc độ hấp thụ/xuất là 168 byte mỗi khối; phần còn lại của trạng thái là capacity 256 bit. Mức an toàn thiết kế của SHAKE128 là 128 bit, nên seed 32 byte không làm cơ chế này trở thành một bộ sinh có mức an toàn 256 bit.

`fips202.c` còn cung cấp SHAKE256, SHA3-256 và SHA3-512; PRNG của chương trình hiện chỉ sử dụng SHAKE128. Mã thư viện có ghi nhận nguồn triển khai trong phần bình luận đầu tệp.

Đây là cơ chế mở rộng seed phục vụ bài lab. Mã hiện chưa có cơ chế tự nạp lại entropy, bảo vệ đầu ra tương lai khi trạng thái bị lộ, hoặc xóa an toàn trạng thái sau khi sử dụng.

### 4.4. Sinh số lớn và chọn đều trong một khoảng

| Hàm trong `bigint.hpp` | Miền giá trị / mục đích |
| --- | --- |
| `randomBigInt<N>` | Sinh toàn bộ `N` phần tử 64 bit, không ép bit cao nhất. |
| `randomBigIntWithBitLength<N>(ctx,b)` | Sinh số có đúng `b` bit, thuộc $[2^{b-1},2^b-1]$. |
| `randomBigIntWithMaxBitLength<N>(ctx,b)` | Sinh số có tối đa `b` bit, thuộc $[0,2^b-1]$. |
| `randomBigIntInRange<N>(ctx,min,max)` | Chọn số trong đoạn đóng `[min,max]` bằng phương pháp lấy mẫu và loại bỏ. |

Hai hàm có tham số độ dài bit sử dụng mặt nạ để bỏ các bit vượt giới hạn. Hàm sinh đúng độ dài đặt thêm bit cao nhất bằng 1. Nếu độ dài bằng 0 hoặc vượt khả năng chứa của kiểu, chúng ném `std::invalid_argument`.

`randomBigIntInRange` tính:

$$
r=\text{max}-\text{min}+1.
$$

Sau đó, hàm sinh `u` với độ dài bit đủ chứa `r`; nếu $u\ge r$ thì loại bỏ và sinh lại. Khi $u<r$, hàm trả về `u + min`.

Nếu luồng bit đầu vào được xem là đều, cách này chọn đều các giá trị trong khoảng. Nó tránh độ lệch do lấy ngay `random_value % r` khi số lượng giá trị nguồn không chia hết cho `r`. Với cách chọn độ dài bit hiện tại, số lần thử kỳ vọng không quá 2.

Hàm hiện giả định `min <= max` và `r` có thể biểu diễn bằng `BigInt<N>`. Khoảng phủ toàn bộ $[0,2^{64N}-1]$ làm `r` bị tràn; trường hợp này cần xử lý riêng nếu muốn xây dựng thư viện dùng tổng quát. Các khoảng dùng trong luồng Diffie–Hellman không phủ toàn bộ miền đó.

### 4.5. Điểm cần lưu ý trong ví dụ PRNG

`main_prng.cpp` dùng seed cố định để minh họa, nhưng chỉ chép chuỗi `"seed"` cùng ký tự kết thúc vào mảng 32 byte:

```cpp
uint8_t seed[32];
memcpy(seed, "seed", sizeof("seed"));
```

Các byte còn lại chưa được khởi tạo, trong khi `prng_seed` đọc toàn bộ 32 byte. Vì vậy, phiên bản này không bảo đảm tạo chuỗi lặp lại ổn định từ seed minh họa. Cách điều chỉnh cho thí nghiệm tái lập là:

```cpp
uint8_t seed[32] = {0};
memcpy(seed, "seed", sizeof("seed"));
```

Đây là đề xuất cải thiện, chưa phải thay đổi đã áp dụng vào mã nguồn. Seed cố định chỉ phù hợp cho thử nghiệm; `main.cpp` đã dùng seed lấy từ hệ điều hành.

## 5. Trao đổi khóa Diffie–Hellman

### 5.1. Các bước thực hiện

Hai bên dùng chung số nguyên tố `p` và phần tử sinh `g` của nhóm nhân $\mathbb{Z}_p^*$:

1. Alice chọn khóa riêng $a\in[2,p-2]$; Bob chọn khóa riêng $b\in[2,p-2]$.
2. Alice tính giá trị công khai $A=g^a\bmod p$; Bob tính $B=g^b\bmod p$.
3. Hai bên trao đổi `A` và `B`.
4. Alice tính $K_A=B^a\bmod p$; Bob tính $K_B=A^b\bmod p$.

Sơ đồ luồng trao đổi:

```mermaid
sequenceDiagram
    participant Alice
    participant Bob
    Note over Alice,Bob: Tham số công khai p, g
    Note over Alice: Giữ bí mật a; tính A = g^a mod p
    Note over Bob: Giữ bí mật b; tính B = g^b mod p
    Alice->>Bob: A
    Bob->>Alice: B
    Note over Alice: K_A = B^a mod p
    Note over Bob: K_B = A^b mod p
    Note over Alice,Bob: K_A = K_B = g^(ab) mod p
```

Hai bí mật chung bằng nhau vì:

$$
K_A=(g^b)^a\bmod p=g^{ab}\bmod p
$$

và:

$$
K_B=(g^a)^b\bmod p=g^{ab}\bmod p.
$$

Người quan sát có thể biết `p`, `g`, `A`, `B`. Tính an toàn dựa trên độ khó của các bài toán trong nhóm được chọn, trong đó bài toán logarit rời rạc là tìm `a` từ $A=g^a\bmod p$; bài toán Diffie–Hellman tính toán là tìm $g^{ab}$ từ $g^a$ và $g^b$.

### 5.2. Chọn phần tử sinh `g`

Khi $p=2q+1$ là số nguyên tố an toàn với $q>2$, nhóm $\mathbb{Z}_p^*$ có bậc $2q$. Bậc của một phần tử là ước của bậc nhóm, nên các khả năng là $1$, $2$, $q$ hoặc $2q$.

Hàm `generate_generator` chọn $g\in[2,p-2]$ và chấp nhận khi:

$$
g^2\not\equiv1\pmod p,\qquad
g^q\not\equiv1\pmod p.
$$

Điều kiện thứ nhất loại bậc 1 và 2; điều kiện thứ hai loại bậc 1 và $q$. Vì vậy, với giả thiết `p` và `q` thực sự nguyên tố, phần tử được chấp nhận có bậc $2q$, tức là sinh toàn bộ nhóm.

Việc chọn ngẫu nhiên rồi kiểm tra tốt hơn việc mặc định `g = 2` cho mọi `p`, vì 2 không luôn là phần tử sinh toàn bộ nhóm. Trong giao thức thực tế, có thể dùng một nhóm con bậc nguyên tố với quy tắc chọn phần tử và kiểm tra khóa công khai tương ứng.

### 5.3. Sinh khóa riêng

Hàm `generate_private_key` gọi:

```cpp
randomBigIntInRange<N>(ctx, BigInt<N>(2), p - BigInt<N>(2));
```

Do đó, khóa riêng nằm đúng trong đoạn đóng $[2,p-2]$ theo đề bài. Alice và Bob lấy hai giá trị liên tiếp từ PRNG đã khởi tạo, thay vì dùng khóa riêng cố định.

### 5.4. Ví dụ nhỏ để kiểm tra bằng tay

Chọn $p=23$, $q=11$, $g=5$, $a=6$ và $b=15$:

| Đại lượng | Phép tính | Kết quả |
| --- | --- | ---: |
| Kiểm tra `g` | $5^2\bmod23$ và $5^{11}\bmod23$ | 2 và 22, đều khác 1 |
| Khóa công khai Alice | $5^6\bmod23$ | 8 |
| Khóa công khai Bob | $5^{15}\bmod23$ | 19 |
| Bí mật chung Alice | $19^6\bmod23$ | 2 |
| Bí mật chung Bob | $8^{15}\bmod23$ | 2 |

Ví dụ này chỉ giải thích nguyên lý. Chương trình chính sử dụng `p` có 512 bit để đáp ứng yêu cầu bài lab.

### 5.5. Ứng dụng thực tế: thiết lập khóa trong TLS

Một ứng dụng của Diffie–Hellman là thiết lập khóa phiên cho kết nối TLS, ví dụ khi truy cập website qua HTTPS. TLS 1.3 hỗ trợ trao đổi khóa Diffie–Hellman trên trường hữu hạn qua các nhóm FFDHE, cũng như biến thể trên đường cong elliptic là ECDHE.

Trong trường hợp sử dụng trao đổi khóa tạm thời:

1. Client và server chọn nhóm được hỗ trợ và trao đổi giá trị công khai.
2. Mỗi bên sử dụng khóa riêng tạm thời để tính bí mật chung.
3. TLS đưa bí mật chung vào quy trình dẫn xuất khóa dùng HKDF, kết hợp với thông tin của phiên bắt tay.
4. Các khóa được dẫn xuất dùng cho cơ chế mã hóa có xác thực để bảo vệ dữ liệu ứng dụng.

Trong bắt tay TLS dùng chứng chỉ, chữ ký và quá trình xác minh chứng chỉ giúp xác thực đối tác và ràng buộc thông tin bắt tay. Diffie–Hellman đứng riêng không cung cấp xác thực, nên có thể bị tấn công xen giữa nếu kẻ tấn công thay thế các giá trị công khai.

Khóa riêng tạm thời và việc loại bỏ chúng sau phiên có thể cung cấp tính bí mật chuyển tiếp: lộ khóa xác thực dài hạn sau này không tự động cho phép giải mã các phiên cũ, với điều kiện các khóa tạm thời không bị lộ.

Các nhóm FFDHE được chuẩn hóa trong RFC 7919 bắt đầu từ 2048 bit. Kích thước 512 bit của bài lab phục vụ học tập và không đủ để bảo vệ thông tin trong ứng dụng thực tế hiện nay. Bí mật chung `K` cũng cần đi qua hàm dẫn xuất khóa, thay vì được dùng trực tiếp làm khóa AES.

## 6. Tổ chức và cách chạy chương trình

### 6.1. Vai trò của các tệp

| Tệp trong `src/` | Vai trò |
| --- | --- |
| `bigint.hpp` | Kiểu số lớn, toán tử, chuyển sang thập phân và các hàm sinh số lớn. |
| `modular.hpp` | Cộng mô-đun tránh tràn, lấy dư, nhân mô-đun, lũy thừa mô-đun. |
| `utils/prime.hpp` | Danh sách nguyên tố nhỏ, bộ lọc và Miller–Rabin. |
| `dh.hpp` | Các hàm ghép thành quy trình Diffie–Hellman. |
| `dh.cpp` | Tệp hiện rỗng; các hàm mẫu được định nghĩa trong `dh.hpp`. |
| `main.cpp` | Chương trình chính: khởi tạo PRNG, sinh tham số, tính các khóa, so sánh và đo thời gian. |
| `main_prng.cpp` | Ví dụ xuất số giả ngẫu nhiên 16, 32 và 64 bit. |
| `lib/prng.h`, `lib/prng.cpp` | Giao diện và cài đặt lấy seed, mở rộng seed bằng SHAKE128. |
| `lib/fips202.h`, `lib/fips202.c` | Giao diện và cài đặt Keccak, SHAKE128/256, SHA3-256/512. |
| `test/bigintTest.cpp` | Kiểm thử số lớn bằng `assert`. |
| `test/modularTest.cpp` | Kiểm thử phép toán mô-đun và mô phỏng Diffie–Hellman với tham số nhỏ. |
| `test/dhTest.cpp` | Sinh số nguyên tố an toàn 64 và 512 bit, in kết quả kiểm tra lại `p` và `q`. |
| `test/test.cpp` | Chương trình thử nghiệm độc lập về nhân schoolbook và Montgomery, có `BigInt` riêng. |
| `u512_sample.h` | Khung mẫu `uint512_t` với khai báo toán tử; không được dùng trong luồng chương trình chính. |
| `Makefile` | Lệnh biên dịch, chạy chương trình và các nhóm kiểm thử. |
| `README-2.md` | Giới thiệu ngắn và cách chạy ví dụ PRNG. |
| `bin/` | Các tệp thực thi đã có trong thư mục; nên biên dịch lại khi mã nguồn thay đổi. |

`test/test.cpp` không được ghép vào `main.cpp`, và không có target tương ứng trong `Makefile`. Vì vậy, chương trình Diffie–Hellman hiện vẫn sử dụng nhân mô-đun theo bit; chưa sử dụng Montgomery trong luồng chính.

Các tệp có hàm `main` là những chương trình độc lập. Không nên biên dịch toàn bộ `.cpp` vào một tệp thực thi bằng wildcard, vì sẽ có nhiều định nghĩa `main`, đồng thời tệp benchmark còn định nghĩa kiểu `BigInt` riêng.

### 6.2. Môi trường biên dịch

Môi trường phù hợp là Linux có `g++` và GNU Make. Trên Windows, có thể dùng Ubuntu trong WSL vì mã PRNG gọi API Linux.

Nếu Ubuntu chưa có công cụ biên dịch, cài trong terminal Ubuntu:

```bash
sudo apt update
sudo apt install build-essential
```

Trong Ubuntu/WSL, chuyển đến thư mục mã nguồn của dự án:

```bash
cd /mnt/d/Hieu/university/3rd_year/HK1/MaHoaMatMa/Lab1/src
```

Trên máy khác, thay đường dẫn bằng vị trí tương ứng. `Makefile` dùng các tùy chọn:

```text
-std=c++14 -O3 -Wall -Wextra -Wpedantic -Wvla -Wredundant-decls
```

`-std=c++14` lựa chọn phiên bản ngôn ngữ, `-O3` bật tối ưu hóa, còn các tùy chọn `-W...` giúp phát hiện vấn đề trong quá trình biên dịch.

### 6.3. Các lệnh chạy

| Lệnh, chạy trong `src/` | Chức năng |
| --- | --- |
| `make` hoặc `make main` | Biên dịch chương trình chính thành `bin/main`. |
| `make run` | Biên dịch và chạy chương trình chính. |
| `make prng` | Biên dịch ví dụ PRNG thành `bin/prng`. |
| `make run-prng` | Biên dịch và chạy ví dụ PRNG. |
| `make test-bigint` | Biên dịch và chạy kiểm thử số lớn. |
| `make test-modular` | Biên dịch và chạy kiểm thử mô-đun. |
| `make test-dh` | Biên dịch và chạy thử sinh số nguyên tố an toàn. |
| `make test-all` | Chạy ba nhóm kiểm thử trên. |
| `make clean` | Xóa thư mục đầu ra `bin/` do quá trình biên dịch tạo ra. |

Lệnh biên dịch thủ công tương đương cho chương trình chính:

```bash
g++ -std=c++14 -O3 -Wall -Wextra -Wpedantic \
    main.cpp lib/prng.cpp lib/fips202.c \
    -I . -I ./lib -pthread -o bin/main
./bin/main
```

Thư mục `bin` cần tồn tại trước khi chạy lệnh biên dịch thủ công; các target trong `Makefile` tự tạo thư mục này.

### 6.4. Luồng của `main.cpp` và ý nghĩa đầu ra

Chương trình chính thực hiện theo thứ tự:

```text
Lấy seed từ hệ điều hành và khởi tạo PRNG
→ Bắt đầu đo thời gian
→ Sinh p có 512 bit và chọn g
→ Sinh khóa riêng a, b
→ Tính khóa công khai A, B
→ Tính bí mật chung của Alice và Bob
→ In các giá trị và kết quả so sánh
→ Kết thúc đo thời gian
```

`Shared keys are equal: true` cho biết hai bí mật chung bằng nhau. `Elapsed time` bao gồm sinh `p`, chọn `g`, sinh khóa, tính các lũy thừa và phần xuất kết quả; không bao gồm bước lấy seed và khởi tạo PRNG trước mốc bắt đầu đo.

Việc in khóa riêng và bí mật chung giúp quan sát bài thực hành. Trong ứng dụng thực tế, các giá trị bí mật này không nên được đưa vào log.

### 6.5. Thay đổi kích thước số nguyên tố

Hiện tại `main.cpp` dùng:

```cpp
int bitSize = 512;
uint512_t p = generate_safe_prime<8>(&ctx, bitSize);
```

Khi đổi sang 1024 bit, cần đồng thời đổi độ dài yêu cầu, kiểu của tất cả các giá trị lớn và tham số mẫu của các lời gọi hàm. Ví dụ thay phần khai báo và tính khóa bằng:

```cpp
int bitSize = 1024;

uint1024_t p = generate_safe_prime<16>(&ctx, bitSize);
uint1024_t g = generate_generator<16>(&ctx, p);
uint1024_t a = generate_private_key<16>(&ctx, p);
uint1024_t b = generate_private_key<16>(&ctx, p);

uint1024_t A = modular_exponentiation<16>(g, a, p);
uint1024_t B = modular_exponentiation<16>(g, b, p);
uint1024_t AliceSharedKey = modular_exponentiation<16>(B, a, p);
uint1024_t BobSharedKey = modular_exponentiation<16>(A, b, p);
```

Muốn dùng 2048 bit, có thể khai báo `using uint2048_t = BigInt<32>;`, dùng kiểu đó cho các giá trị và chọn tham số mẫu `<32>`.

Không chỉ đổi `bitSize` thành 1024 trong khi giữ `BigInt<8>`: hàm sinh `q` sẽ yêu cầu vượt dung lượng và ném ngoại lệ. Cần bảo đảm `bitSize <= 64N`; riêng hàm sinh số nguyên tố an toàn nên kiểm tra thêm `bitSize >= 3`. Bản nộp theo đề phải sử dụng `bitSize >= 512`; cấu hình 64 bit trong bài test chỉ phục vụ kiểm tra nhanh.

Những đoạn thay đổi trên là hướng dẫn cấu hình. Kết quả thực nghiệm trong báo cáo được đo ở cấu hình 512 bit; chưa đo thời gian cho cấu hình 1024 hoặc 2048 bit.

### 6.6. Thay đổi số vòng Miller–Rabin

Có thể đổi giá trị mặc định `iterations = 20` trong `utils/prime.hpp`. Vì `generate_safe_prime` không truyền đối số số vòng, nó sẽ sử dụng giá trị mặc định mới.

Nếu muốn chỉ điều chỉnh khi sinh số nguyên tố an toàn, truyền đối số thứ tư vào cả hai lời gọi:

```cpp
isPrime(q, ctx, false, 40);
isPrime(p, ctx, false, 40);
```

Trong mã thực tế, các lời gọi này nằm trong điều kiện loại ứng viên; cần giữ nguyên logic đó khi chỉnh sửa. Tăng số vòng làm giảm chặn sai số nhưng tăng thời gian kiểm tra các ứng viên vượt qua những vòng đầu.

## 7. Kiểm thử và kết quả thực nghiệm

### 7.1. Môi trường và phương pháp

Các kết quả dưới đây được thu thập ngày 17/11/2026 bằng Ubuntu trong WSL2 trên Windows, GCC 13.3.0, biên dịch với C++14 và tối ưu hóa `-O3`.

Mã nguồn được sao chép sang thư mục tạm để biên dịch và kiểm tra, nhằm giữ nguyên các tệp nguồn và tệp thực thi hiện có trong dự án. Kiểm thử số lớn và mô-đun chạy qua các target của `Makefile`; `dhTest` được biên dịch với cùng các nguồn phụ thuộc rồi chạy riêng. Thời gian của chương trình chính và bài thử sinh nguyên tố được đo trong một lượt chạy có hai tiến trình đồng thời, nên chỉ dùng làm số liệu minh họa.

### 7.2. Các kiểm thử đã thực hiện

| Thành phần | Nội dung kiểm tra | Kết quả |
| --- | --- | --- |
| `bigintTest` | Cộng, số nhớ qua phần tử 64 bit, tràn cuối mảng, trừ và số mượn, so sánh, thao tác bit, dịch qua ranh giới phần tử, xuất thập phân | Tất cả nhóm kiểm thử có sẵn đều `PASSED`. |
| `modularTest` | `Mod`, `AddMod`, `MulMod`, `ExpMod`, số mũ 0/1, quan hệ Fermat, mô phỏng Diffie–Hellman với tham số nhỏ | Tất cả nhóm kiểm thử có sẵn đều `PASSED`. |
| Biên dịch `main` và `prng` | Biên dịch các chương trình bằng C++14 | Thành công. Ví dụ PRNG chưa được dùng để khẳng định tính tái lập. |
| `dhTest`, 64 bit | Sinh `p`, tính `q`, kiểm tra lại cả hai bằng `isPrime` | `TRUE` cho cả `p` và `q`. |
| `dhTest`, 512 bit | Sinh `p`, tính `q`, kiểm tra lại cả hai bằng `isPrime` | `TRUE` cho cả `p` và `q`. |
| Chương trình chính 512 bit | Sinh tham số và tính bí mật chung của Alice, Bob | `Shared keys are equal: true`. |
| Đối chiếu bằng Python | Kiểm tra độ dài bit, khoảng giá trị, điều kiện `g` và bốn phép lũy thừa tạo khóa | Tất cả phép đối chiếu đều đúng. |

Các kiểm thử có sẵn dùng nhiều giá trị nhỏ được lưu trong kiểu 512 bit. Chúng kiểm tra những trường hợp cụ thể, chưa bao phủ toàn bộ miền 512 bit. `dhTest` in kết quả kiểm tra nguyên tố nhưng không dùng `assert` để bắt buộc các giá trị đó phải là `TRUE`; khi đọc kết quả cần kiểm tra nội dung in ra, không chỉ mã thoát của chương trình.

Kiểm tra lại bằng `isPrime` vẫn là Miller–Rabin và dùng cùng cài đặt, nên không phải chứng nhận nguyên tố độc lập. Đối chiếu Python được thực hiện cho phép toán và điều kiện chọn tham số, không chứng nhận tính nguyên tố của `p` và `q`.

### 7.3. Kết quả các bài thử

Trong kiểm thử mô-đun, mô phỏng với $p=1000000007$, $g=5$, $a=123456$, $b=654321$ cho:

```text
Diffie-Hellman Key Exchange Simulation...
PASSED (Shared Secret: 694764134)
```

Một lần chạy `dhTest` sinh được:

```text
p (64-bit): 16185944117712469847
q = (p-1)/2: 8092972058856234923
isPrime(p): TRUE
isPrime(q): TRUE
Thoi gian: 0.258278 giay
```

Với cấu hình 512 bit trong cùng bài thử:

```text
p (512-bit): 10257915591924790768883679707854723460501755545282727801807369523275822263498818296935190980185685730021853497525867184171195165881224184867966912566429383
q = (p-1)/2: 5128957795962395384441839853927361730250877772641363900903684761637911131749409148467595490092842865010926748762933592085597582940612092433983456283214691
isPrime(p): TRUE
isPrime(q): TRUE
Thoi gian: 23.6946 giay
```

Ở `dhTest`, thời gian được đo quanh lời gọi `generate_safe_prime`, trước phần xuất số và kiểm tra lại bằng `isPrime`.

### 7.4. Kết quả chương trình chính 512 bit

Đầu ra của một lượt chạy thực tế:

```text
Safe prime p: 9342081910161219128152558738785837101774319074404913757107222814154856037761323062450822562952293150601752806351193028833165029540445941613845751719007703
Generator g: 2957158361377389539282958154639818968715296277904446484616493593381097894480550727869117004139438863851690596707002937769297973776892477662569024897317929
Alice's private key a: 2342172373212306310753441247543211429328447057075373031149303254872508613972089564790181900574020789231711824007916196091963707786023333892629853121992561
Bob's private key b: 8624057326185832383829129846386006837404144665700900801884420355013867854476385242531464620096833007763808275786698962836543735327726041387422203140468958
Alice's public key A: 39265769963542231224960995648167246580794963800537200898429679308118985600491624840441762236971111795334893617410191901221540337620136203781436005393143
Bob's public key B: 7700838380710697886659242176944402051941447358481116757668132569948049016892109737942485555015536508984326889926107281826581877792973596711939632730036550
Alice's shared key: 6248355613148129415612882271721359280554642553577396482609535891227651600403318998306841205729495164370697476424344976212100718489395474593432007348086130
Bob's shared key: 6248355613148129415612882271721359280554642553577396482609535891227651600403318998306841205729495164370697476424344976212100718489395474593432007348086130
Shared keys are equal: true
Elapsed time: 53.0277 seconds
```

Đối chiếu bằng số nguyên độ dài tùy ý và `pow(base, exponent, modulus)` của Python xác nhận:

$$
\operatorname{bit\_length}(p)=512,
\quad 2\le a,b,g\le p-2,
$$

$$
g^2\bmod p\ne1,\quad g^{(p-1)/2}\bmod p\ne1,
$$

và các giá trị `A`, `B`, bí mật chung của hai bên đều khớp với phép tính Python.

Số liệu thời gian không phải giá trị cố định: quá trình tìm ứng viên ngẫu nhiên, tải hệ thống và môi trường chạy đều ảnh hưởng đến kết quả. Không thể dùng một lượt chạy để kết luận tốc độ trung bình hay so sánh chính xác các thuật toán.

## 8. Tự đánh giá và hướng cải thiện

### 8.1. Ưu điểm

- **Đáp ứng các chức năng chính của bài lab:** có số lớn, lũy thừa mô-đun, sinh số nguyên tố an toàn, chọn khóa riêng và mô phỏng Diffie–Hellman 512 bit.
- **Cách biểu diễn dễ mở rộng:** mẫu `BigInt<N>` cho phép thay số phần tử để tăng dung lượng mà không phải viết lại toàn bộ toán tử.
- **Tránh tràn trong phép toán mô-đun:** `addReduced` giúp giữ các giá trị trong giới hạn, còn `MulMod` không cần tạo tích đầy đủ.
- **Loại hợp số sớm:** bộ lọc nguyên tố nhỏ giảm chi phí Miller–Rabin; kiểm tra dừng ngay khi tìm được nhân chứng hợp số.
- **Chọn tham số có cơ sở toán học:** `g` được kiểm tra theo phân tích $p-1=2q$, khóa riêng được lấy đúng khoảng bằng lấy mẫu và loại bỏ.
- **Có kiểm thử và đối chiếu thực nghiệm:** các bài test có sẵn đã chạy thành công, kết quả Diffie–Hellman được so sánh thêm với Python.

### 8.2. Hạn chế

| Hạn chế                                                                 | Tác động                                                                                                                         |
| ----------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| Nhân mô-đun theo từng bit                                               | Sinh số nguyên tố an toàn và lũy thừa với khóa lớn có thể chậm.                                                                  |
| Dung lượng số lớn cố định, không báo lỗi tràn cộng hoặc hụt trừ         | Người dùng thư viện phải tự bảo đảm các bất biến của thuật toán.                                                                 |
| Một số đầu vào chưa được kiểm tra đầy đủ                                | Cần xử lý mô-đun 0, khoảng ngẫu nhiên không hợp lệ, khoảng toàn miền và `bitsize` không phù hợp.                                 |
| `isPrime` có đối số mặc định `ctx = nullptr`                            | Nếu đầu vào cần đến bước Miller–Rabin, mã sẽ dùng con trỏ này để sinh cơ số; phải truyền trạng thái PRNG hợp lệ.                 |
| Chưa ngăn `iterations = 0`                                              | Số lẻ vượt bộ lọc có thể được chấp nhận mà không chạy vòng Miller–Rabin nào.                                                     |
| Seed minh họa trong `main_prng.cpp` chưa được khởi tạo đầy đủ           | Ví dụ chưa bảo đảm tính tái lập của luồng đầu ra.                                                                                |
| `randombytes` dùng `uint8_t outlen`                                     | Độ dài chỉ biểu diễn đến 255 byte; nhu cầu lấy nhiều byte hơn cần đổi sang `size_t`. Lời gọi 32 byte hiện tại nằm trong phạm vi. |
| Phụ thuộc `getrandom` và kiểu nguyên 128 bit mở rộng                    | Không bảo đảm biên dịch trực tiếp trên mọi hệ điều hành hoặc trình biên dịch C++14.                                              |
| Chưa có xác thực, dẫn xuất khóa và kiểm tra khóa công khai từ bên ngoài | Mới thể hiện nguyên lý trao đổi khóa, chưa đủ thành một giao thức bảo vệ dữ liệu thực tế.                                        |
| Nhánh xử lý phụ thuộc bit số mũ và có in giá trị bí mật                 | Có nguy cơ lộ thông tin qua thời gian chạy hoặc log nếu áp dụng trong môi trường thực tế.                                        |
| Kích thước 512 bit                                                      | Đáp ứng đề bài nhưng không đạt mức an toàn cần thiết cho triển khai hiện đại.                                                    |

Trong luồng hiện tại, `p`, `g` và khóa công khai đều được tạo nội bộ. Nếu phát triển thành hai bên trao đổi qua mạng, cần kiểm tra tham số và khóa công khai nhận được theo nhóm đã chọn, đồng thời xác thực đối tác.

### 8.3. Cải thiện hiệu năng

`test/test.cpp` đã có mã thử nghiệm cho hai hướng:

1. **Nhân schoolbook theo phần tử 64 bit:** tạo tích 1024 bit từ hai số 512 bit, dùng các biến trung gian 128 bit để giữ số nhớ.
2. **Nhân Montgomery theo CIOS:** với mô-đun lẻ, tính $abR^{-1}\bmod m$, trong đó $R=2^{512}$, thông qua xử lý theo phần tử và rút gọn mô-đun mà không cần phép chia số lớn tổng quát.

Nhân Montgomery cần chuyển toán hạng vào miền Montgomery và chuyển kết quả ra đúng cách khi ghép vào lũy thừa. Không thể thay trực tiếp `MulMod(a,b,m)` bằng `montgomery_mul(a,b,m,...)` vì hai hàm tính hai biểu thức khác nhau.

Benchmark hiện tại chưa dùng kết quả tính để tạo đầu ra có thể quan sát, nên trình biên dịch tối ưu có thể loại bỏ công việc trong vòng lặp. Hai phép đo cũng khác ý nghĩa: schoolbook trả tích đầy đủ, Montgomery trả một kết quả nhân kèm rút gọn. Do đó, báo cáo không sử dụng các số đo từ tệp này làm bằng chứng rằng Montgomery đã cải thiện chương trình chính. Khẳng định trong bình luận rằng một phép nhân chỉ mất một chu kỳ CPU cũng không nên xem là bảo đảm cho mọi nền tảng.

Để đánh giá cải tiến, cần tích hợp đúng thuật toán, kiểm tra với dữ liệu tham chiếu, tạo benchmark giữ lại công việc cần đo và so sánh các phép toán có cùng mục đích. Có thể đo riêng thời gian sinh `p`, tìm `g` và tính khóa, rồi thực hiện nhiều lượt để báo cáo phân bố thời gian.

### 8.4. Cải thiện độ tin cậy và tính an toàn

Các hướng ưu tiên gồm kiểm tra điều kiện đầu vào, yêu cầu PRNG hợp lệ khi kiểm tra số lớn, khởi tạo toàn bộ seed minh họa và đưa số vòng Miller–Rabin thành cấu hình có giới hạn hợp lý.

Nếu mở rộng thử nghiệm, nên đối chiếu phép cộng, lấy dư, nhân và lũy thừa trên nhiều giá trị gần giới hạn 512 bit; thêm trường hợp biên cho độ dài bit và khoảng ngẫu nhiên; kiểm tra các hợp số được chọn để thử Miller–Rabin. Các test sinh nguyên tố cũng nên dùng điều kiện kiểm tra bắt buộc, thay vì chỉ in `TRUE` hoặc `FALSE`.

Nếu hướng tới ứng dụng thực tế, cần dùng nhóm và kích thước được chuẩn hóa, xác thực phiên trao đổi, dẫn xuất khóa từ bí mật chung và bảo vệ các giá trị bí mật. Phép lũy thừa cần được thiết kế để hạn chế rò rỉ qua thời gian thực thi; việc sử dụng thư viện mật mã đã được kiểm chứng phù hợp hơn tự phát triển toàn bộ cơ chế cho sản phẩm.

## 9. Tài liệu tham khảo

1. `Lab1.pdf`, đề bài “Số nguyên tố & trao đổi khóa Diffie-Hellman”, Bộ môn Công nghệ Tri thức, đặc biệt mục 2.5 về báo cáo và mục 3 về quy định nộp bài.
2. Mã nguồn trong thư mục `src/`, gồm các tệp cài đặt, thư viện cung cấp và chương trình kiểm thử.
3. Stephen D. Galbraith, *Mathematics of Public Key Cryptography*, chương 20: [The Diffie–Hellman Protocol](https://www.math.auckland.ac.nz/~sgal018/crypto-book/ch20.pdf), tài liệu được dẫn trong đề bài.
4. Stanford PBC Notes: [Miller–Rabin](https://crypto.stanford.edu/pbc/notes/numbertheory/millerrabin.html).
5. PlanetMath: [Safe prime](https://planetmath.org/SafePrime).
6. NIST, [FIPS 202: SHA-3 Standard — Permutation-Based Hash and Extendable-Output Functions](https://doi.org/10.6028/NIST.FIPS.202).
7. [RFC 8446 — The Transport Layer Security (TLS) Protocol Version 1.3](https://www.rfc-editor.org/rfc/rfc8446), đặc biệt các mục về trao đổi khóa và dẫn xuất khóa.
8. [RFC 7919 — Negotiated Finite Field Diffie–Hellman Ephemeral Parameters for TLS](https://www.rfc-editor.org/rfc/rfc7919).
9. [RFC 5869 — HMAC-based Extract-and-Expand Key Derivation Function (HKDF)](https://www.rfc-editor.org/rfc/rfc5869).
10. Linux man-pages: [getrandom(2)](https://man7.org/linux/man-pages/man2/getrandom.2.html).

## Phụ lục: Chuẩn bị báo cáo để nộp

Đây là bản báo cáo Markdown để chỉnh sửa. Theo mục 3 của `Lab1.pdf`, báo cáo trong bài nộp phải ở định dạng PDF.

1. Điền họ tên, MSSV; xóa những hàng thành viên không sử dụng.
2. Dùng công cụ hỗ trợ Markdown, công thức toán và Mermaid để xuất PDF. Nếu công cụ không hỗ trợ Mermaid, chuyển sơ đồ thành hình trước khi xuất hoặc dùng các bước trao đổi đã mô tả ở mục 5.1.
3. Kiểm tra công thức, bảng và các dòng số 512 bit trong PDF; bật tự xuống dòng cho khối mã dài nếu cần.
4. Đặt mã nguồn và PDF vào cấu trúc theo đề:

```text
<MSSV1-MSSV2-...>.zip
├── project_01_source/
│   ├── main.cpp
│   ├── bigint.hpp
│   ├── modular.hpp
│   ├── dh.hpp
│   ├── dh.cpp
│   ├── Makefile
│   ├── lib/
│   ├── utils/
│   └── test/
└── project_01_report/
    └── Lab1_Report.pdf
```

Nhóm tối đa 4 sinh viên; các MSSV trong tên ZIP cần được sắp theo thứ tự trong danh sách lớp như đề yêu cầu. Khi sao chép mã nguồn, giữ đủ các tệp tiêu đề và thư viện phụ thuộc để chương trình có thể biên dịch lại.
