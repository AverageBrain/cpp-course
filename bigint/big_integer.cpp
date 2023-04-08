#include "big_integer.h"
#include <cstddef>
#include <cstring>
#include <ostream>
#include <stdexcept>

static const int64_t BASE = UINT32_MAX + 1ULL;
static const uint32_t DIGIT_BASE = 1000000000;

bool big_integer::is_zero() const {
  return (this->number.empty());
}

big_integer::big_integer() :
      sign(false)
      {}

big_integer::big_integer(big_integer const& other) = default;

void big_integer::fill_vector(uint64_t a) {
  int i = 0;
  while (a > 0) {
    number[i] = a % BASE;
    a /= (uint64_t) BASE;
    i++;
  }
  cut_leading_zero(*this);
}

big_integer::big_integer(int a) : sign(a < 0), number(2) {
  fill_vector(std::abs((int64_t)a));
}

big_integer::big_integer(unsigned a) : sign(false), number(2) {
  fill_vector(a);
}

big_integer::big_integer(long a) :
      big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(unsigned long a) :
      big_integer(static_cast<unsigned long long>(a)) {}

big_integer::big_integer(long long a) : sign(a < 0), number(3) {
  if (a == INT64_MIN) {
    fill_vector((uint64_t)(INT64_MAX + 1ULL));
  } else {
    fill_vector(std::abs(a));
  }
}

big_integer::big_integer(unsigned long long a) : sign(false), number(3) {
  fill_vector(a);
}

big_integer::big_integer(const std::string& str) : big_integer() {
  if (str.empty() || str == "-") {
    throw std::invalid_argument("Invalid number");
  }
  bool is_neg = str[0] == '-';
  for (size_t i = is_neg; i < str.size(); i++) {
    if (!('0' <= str[i] && str[i] <= '9')) {
      throw std::invalid_argument("Invalid number");
    }
  }

  size_t i = is_neg;
  for (; i + 9 <= str.size(); i += 9) {
    *this *= DIGIT_BASE;
    *this += std::stoi(str.substr(i, 9));
  }
  size_t len_last_digit = (str.size() - is_neg) % 9;
  if (len_last_digit > 0) {
    uint32_t power_10 = 1;
    while (len_last_digit > 0) {
      power_10 *= 10;
      len_last_digit--;
    }
    *this *= power_10;
    *this += std::stoi(str.substr(i));
  }
  if (is_neg)
    this->sign = true;
  cut_leading_zero(*this);
}

big_integer::~big_integer() = default;

void big_integer::swap(big_integer& a) {
  std::swap(number, a.number);
  std::swap(sign, a.sign);
}

big_integer& big_integer::operator=(big_integer const& other) {
  if (&other != this) {
    big_integer(other).swap(*this);
  }
  return *this;
}

void big_integer::cut_leading_zero(big_integer& num) {
  size_t i = num.number.size();
  while (i > 0 && num.number[i - 1] == 0) {
    num.number.pop_back();
    i--;
  }
}

void big_integer::add_abs(big_integer& res, const big_integer& a, const big_integer& b) {
  uint64_t carry = 0;
  size_t max_size = std::max(a.number.size(), b.number.size());
  res.number.resize(max_size + 1);
  for (size_t i = 0; i < max_size; ++i) {
    uint64_t cur_bit = (uint64_t) (i < a.number.size() ? a.number[i] : 0) +
                       (uint64_t) (i < b.number.size() ? b.number[i] : 0) +
                       carry;
    carry = cur_bit / BASE;
    res.number[i] = cur_bit - carry * BASE;
  }
  res.number[max_size] = carry;
  cut_leading_zero(res);
}

big_integer& big_integer::operator+=(big_integer const& rhs) {
  if (sign == rhs.sign) {
    add_abs(*this, *this, rhs);
  } else {
    if (comp_abs_less(rhs)) {
      sub_abs(*this, rhs, *this);
      this->sign = rhs.sign;
    } else {
      sub_abs(*this, *this, rhs);
      this->sign = !rhs.sign;
    }
  }
  return *this;
}

void big_integer::sub_abs(big_integer& diff, const big_integer& a, const big_integer& b) {
  int64_t borrow = 0;
  size_t max_size = std::max(a.number.size(), b.number.size());
  diff.number.resize(max_size);
  for (size_t i = 0; i < max_size; ++i) {
    int64_t cur_bit = (int64_t) (i < a.number.size()? a.number[i]: 0) -
                      (int64_t) (i < b.number.size() ? b.number[i] : 0) +
                      borrow;
    if (cur_bit < 0) {
      cur_bit += BASE;
      borrow = -1;
    } else {
      borrow = 0;
    }
    diff.number[i] = cur_bit;
  }
  cut_leading_zero(diff);
}

big_integer& big_integer::operator-=(big_integer const& rhs) {
  if (sign != rhs.sign) {
    add_abs(*this, *this, rhs);
    this->sign = !rhs.sign;
    return *this;
  } else {
      if ((*this > rhs) ^ sign) {
        sub_abs(*this, *this, rhs);
        this->sign = rhs.sign;
      } else {
        sub_abs(*this, rhs, *this);
        this->sign = !rhs.sign;
      }
  }
  cut_leading_zero(*this);
  return *this;
}

big_integer big_integer::mul_bigint_bigint(const big_integer& a, const big_integer& b) {
  big_integer res;
  res.number.resize(a.number.size() + b.number.size() + 1);
  for (size_t i = 0; i < a.number.size(); ++i){
    uint64_t carry = 0;
    for (size_t j = 0; j < b.number.size(); ++j){
      uint64_t cur_bit = (uint64_t) a.number[i] * (uint64_t) (j < b.number.size()? b.number[j]: 0ULL) + res.number[i + j] + carry;
      carry = (cur_bit) / BASE;
      res.number[i + j] = cur_bit - BASE * carry;
    }
    if (carry > 0)
      res.number[i + b.number.size()] += carry;
  }
  res.sign = a.sign ^ b.sign;
  cut_leading_zero(res);
  return res;
}

big_integer& big_integer::operator*=(big_integer const& rhs) {
  mul_bigint_bigint(*this, rhs).swap(*this);
  return *this;
}

uint64_t big_integer::trial(const big_integer& a, const big_integer& b, size_t k, size_t m) {
  size_t km = k + m;
  if (a == 0) return 0;
  uint64_t r2 = ((uint64_t )a.number[km] * BASE +
                (uint64_t )a.number[km - 1]) ;
  uint64_t d1 = (uint64_t )b.number[m - 1] ;
  return std::min((r2 / d1), (uint64_t)(BASE - 1));
}

bool big_integer::smaller(const big_integer& r, const big_integer& dq, size_t k, size_t m) {
  size_t i = m, j = 0;
  while (i != j) {
    if (r.number[i + k] != dq.number[i]) j = i;
    else i--;
  }
  return r.number[i + k] < dq.number[i];
}

std::pair<big_integer, big_integer> big_integer::long_divide(big_integer const& a, big_integer const& b) {
  uint64_t f = BASE / (b.number.back() + 1);
  big_integer r = a * f;
  big_integer d = b * f;
  big_integer q;
  q.number.resize(a.number.size() - b.number.size() + 2);
  r.number.push_back(0);
  for (size_t k = a.number.size() - b.number.size() + 1; k > 0; k--) {
    uint64_t qt = trial(r, d, k - 1, b.number.size());
    big_integer dq = d * qt;
   if (qt == 0) continue;
    dq.number.push_back(0);
    while (qt != 0 && smaller(r, dq, k - 1, b.number.size())) {
      qt--;
      dq -= d;
    }
    q.number[k - 1] = qt;

    uint64_t borrow = 0;
    for (size_t i = 0; i <= b.number.size(); i++) {
      uint64_t diff = (uint64_t)r.number[i + k - 1] - dq.number[i] - borrow + BASE;
      r.number[i + k - 1] = diff % BASE;
      borrow = 1 - diff / BASE;
    }
  }
  cut_leading_zero(q);
  r = divide_long_short(r, f);
  cut_leading_zero(r);
  return {q, r};
}

big_integer big_integer::divide_long_short(big_integer const&a, uint32_t b) {
  big_integer res;
  res.number.resize(a.number.size());
  uint64_t carry = 0;
  for (size_t i = a.number.size(); i > 0; i--) {
    uint64_t temp = carry * BASE + a.number[i - 1];
    res.number[i - 1] = temp / b;
    carry = temp % b;
  }
  cut_leading_zero(res);
  return res;
}

big_integer& big_integer::operator/=(big_integer const& rhs) {
  if (rhs == 0) {
    throw std::runtime_error("Division by zero");
  }
  bool div_sign = (this->sign ^ rhs.sign);
  if (rhs.number.size() == 1) {
    divide_long_short(*this, rhs.number[0]).swap(*this);
    if (!this->is_zero()) {
      this->sign = div_sign;
    }
    return *this;
  }
  if (this->number.size() < rhs.number.size()) {
    return (*this = 0);
  } else {
    long_divide(*this, rhs).first.swap(*this);
    this->sign = div_sign;
    return *this;
  }
}

big_integer big_integer::remainder_long_short(const big_integer& a, uint32_t b) {
  uint64_t carry = 0;
  for (size_t i = a.number.size(); i > 0; i--) {
    carry = (carry * BASE + a.number[i - 1]) % b;
  }
  big_integer res(carry);
  res.sign = a.sign;
  return res;
}

big_integer& big_integer::operator%=(big_integer const& rhs) {
  if (rhs == 0) {
    throw std::runtime_error("Division by zero");
  }
  bool div_sign = this->sign;
  if (rhs.number.size() == 1) {
    remainder_long_short(*this, rhs.number[0]).swap(*this);
    if (!this->is_zero()) {
      this->sign = div_sign;
    }
    return *this;
  }
  if (comp_abs_less(rhs)) {
    return (*this);
  } else {
    long_divide(*this, rhs).second.swap(*this);
    this->sign = div_sign;
    cut_leading_zero(*this);
    return *this;
  }
}

// two's complement for a -> ~a + 1

uint32_t big_integer::get_pos(big_integer const& a, size_t pos) {
  if (!a.sign) {
    if (pos < a.number.size())
      return a.number[pos];
    else
      return 0;
  } else {
    if (pos < a.number.size())
      return ~(a.number[pos]);
    else
      return UINT32_MAX;
  }
}

big_integer big_integer::bit_operation(big_integer const& a, big_integer const& b,
                                        uint32_t(*oper)(uint32_t, uint32_t)) {
  size_t max_size = std::max(a.number.size(), b.number.size());
  big_integer x = a;
  if (x.sign) {
    ++x;
    x.sign = true;
  }
  big_integer y = b;
  if (y.sign) {
    ++y;
    y.sign = true;
  }

  number.resize(max_size);
  for (size_t i = 0; i < max_size; i++) {
    number[i] = oper(get_pos(x, i), get_pos(y, i));
  }
  sign = oper(x.sign, y.sign);
  if (sign) {
    for (size_t i = 0; i < max_size; i++) {
      number[i] = ~number[i];
    }
    --(*this);
  }
  cut_leading_zero(*this);
  return *this;
}

big_integer& big_integer::operator&=(big_integer const& rhs) {
  return *this = bit_operation(*this, rhs, [](uint32_t a, uint32_t b)->uint32_t{
    return a & b;
  });
}

big_integer& big_integer::operator|=(big_integer const& rhs) {
  return *this = bit_operation(*this, rhs, [](uint32_t a, uint32_t b)->uint32_t{
    return a | b;
  });
}

big_integer& big_integer::operator^=(big_integer const& rhs) {
  return *this = bit_operation(*this, rhs, [](uint32_t a, uint32_t b)->uint32_t{
    return a ^ b;
  });
}

big_integer& big_integer::operator<<=(int rhs) {
  int full_bits = rhs / 32;
  int shift = rhs % 32;
  *this *= (1Ull << shift);
  for (size_t i = 0; i < full_bits; i++) {
    number.insert(number.begin(), 0);
  }
  cut_leading_zero(*this);
  return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
  int full_bits = rhs / 32;
  for (size_t i = 0; i < full_bits && !number.empty(); i++) {
    number.erase(number.begin());
  }
  if (number.empty()) {
    *this = 0;
  }
  int shift = rhs % 32;
  *this /= (1ULL << shift);
  if (sign) {
    --(*this);
  }
  cut_leading_zero(*this);
  return *this;
}

big_integer big_integer::operator+() const {
  return *this;
}

big_integer big_integer::operator-() const {
  big_integer res(*this);
  res.sign = !res.sign;
  return res;
}

big_integer big_integer::operator~() const {
  return -(*this) - 1;
}

big_integer& big_integer::operator++() {
  return (*this += 1);
}

big_integer big_integer::operator++(int) {
  big_integer copy = *this;
  ++(*this);
  return copy;
}

big_integer& big_integer::operator--() {
  return (*this -= 1);
}

big_integer big_integer::operator--(int) {
  big_integer copy = *this;
  --(*this);
  return copy;
}

big_integer operator+(big_integer a, big_integer const& b) {
  return a += b;
}

big_integer operator-(big_integer a, big_integer const& b) {
  return a -= b;
}

big_integer operator*(big_integer a, big_integer const& b) {
  return a *= b;
}

big_integer operator/(big_integer a, big_integer const& b) {
  return a /= b;
}

big_integer operator%(big_integer a, big_integer const& b) {
  return a %= b;
}

big_integer operator&(big_integer a, big_integer const& b) {
  return a &= b;
}

big_integer operator|(big_integer a, big_integer const& b) {
  return a |= b;
}

big_integer operator^(big_integer a, big_integer const& b) {
  return a ^= b;
}

big_integer operator<<(big_integer a, int b) {
  return a <<= b;
}

big_integer operator>>(big_integer a, int b) {
  return a >>= b;
}

bool operator==(big_integer const& a, big_integer const& b) {
  return (a.sign == b.sign && a.number == b.number) ||
         (a.is_zero() && b.is_zero());
}

bool operator!=(big_integer const& a, big_integer const& b) {
  return !(a == b);
}

bool big_integer::comp_abs_less(const big_integer& other) const {
  if (number.size() != other.number.size())
    return (number.size() < other.number.size());
  for (size_t i = number.size(); i > 0; i--) {
    if (number[i - 1] != other.number[i - 1]) {
      return (number[i - 1] < other.number[i - 1]);
    }
  }
  return sign;
}

bool operator<(big_integer const& a, big_integer const& b) {
  if (a.sign != b.sign)
    return a.sign;
  return a.sign ^ a.comp_abs_less(b);
}

bool operator>(big_integer const& a, big_integer const& b) {
  return b < a;
}

bool operator<=(big_integer const& a, big_integer const& b) {
  return !(a > b);
}

bool operator>=(big_integer const& a, big_integer const& b) {
  return !(a < b);
}

std::string to_string(big_integer const& a) {
  std::string ans = "";
  if (a.is_zero()) {
    return "0";
  }
  big_integer temp = a;
  while (!temp.is_zero()) {
    big_integer mod = temp % DIGIT_BASE;
    uint32_t digit_mod = (!mod.is_zero() ? mod.number[0] : 0);
    for (size_t i = 0; i < 9; i++) {
      ans = (char) ((digit_mod % 10) + '0') + ans;
      digit_mod /= 10;
    }
    temp = temp.divide_long_short(temp, DIGIT_BASE);
  }
  while (ans[0] == '0') {
    ans.erase(ans.begin());
  }
  if (a.sign) {
    ans = '-' + ans;
  }
  return ans;
}

std::ostream& operator<<(std::ostream& s, big_integer const& a) {
  return s << to_string(a);
}
