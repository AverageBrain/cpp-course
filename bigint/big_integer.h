#pragma once

#include <iosfwd>
#include <vector>
#include <string>

struct big_integer {
  big_integer();
  big_integer(big_integer const& other);

  big_integer(int a);
  big_integer(unsigned a);
  big_integer(long a);
  big_integer(unsigned long a);
  big_integer(long long a);
  big_integer(unsigned long long a);

  explicit big_integer(std::string const& str);
  ~big_integer();

  big_integer& operator=(big_integer const& other);

  big_integer& operator+=(big_integer const& rhs);
  big_integer& operator-=(big_integer const& rhs);
  big_integer& operator*=(big_integer const& rhs);
  big_integer& operator/=(big_integer const& rhs);
  big_integer& operator%=(big_integer const& rhs);

  big_integer& operator&=(big_integer const& rhs);
  big_integer& operator|=(big_integer const& rhs);
  big_integer& operator^=(big_integer const& rhs);

  big_integer& operator<<=(int rhs);
  big_integer& operator>>=(int rhs);

  big_integer operator+() const;
  big_integer operator-() const;
  big_integer operator~() const;

  big_integer& operator++();
  big_integer operator++(int);

  big_integer& operator--();
  big_integer operator--(int);

  friend bool operator==(big_integer const& a, big_integer const& b);
  friend bool operator!=(big_integer const& a, big_integer const& b);
  friend bool operator<(big_integer const& a, big_integer const& b);
  friend bool operator>(big_integer const& a, big_integer const& b);
  friend bool operator<=(big_integer const& a, big_integer const& b);
  friend bool operator>=(big_integer const& a, big_integer const& b);

  friend std::string to_string(big_integer const& a);

private:
  void swap(big_integer &);
  void cut_leading_zero(big_integer &);
  bool is_zero() const;
  void fill_vector(uint64_t);
  bool comp_abs_less(big_integer const&) const;

  // for sum and subtract

  void add_abs(big_integer &, big_integer const&, big_integer const&);
  void sub_abs(big_integer &, big_integer const&, big_integer const&);

  // for multiply

  big_integer mul_bigint_bigint(big_integer const& a, big_integer const& b);

  // for division

  std::pair<big_integer, big_integer> long_divide(big_integer const& a, big_integer const& b);
  uint64_t trial(big_integer const& a, big_integer const& b, size_t k, size_t m);
  bool smaller(big_integer const& r, big_integer const& dq, size_t k, size_t m);
  big_integer divide_long_short(big_integer const &a, uint32_t b);
  big_integer remainder_long_short(big_integer const &a, uint32_t b);

  // for bit_operations

  uint32_t get_pos(big_integer const&, size_t);
  big_integer bit_operation(big_integer const& a, big_integer const& b,
                             uint32_t(*oper)(uint32_t, uint32_t));

  bool sign;
  std::vector<uint32_t> number;
};

big_integer operator+(big_integer a, big_integer const& b);
big_integer operator-(big_integer a, big_integer const& b);
big_integer operator*(big_integer a, big_integer const& b);
big_integer operator/(big_integer a, big_integer const& b);
big_integer operator%(big_integer a, big_integer const& b);

big_integer operator&(big_integer a, big_integer const& b);
big_integer operator|(big_integer a, big_integer const& b);
big_integer operator^(big_integer a, big_integer const& b);

big_integer operator<<(big_integer a, int b);
big_integer operator>>(big_integer a, int b);

bool operator==(big_integer const& a, big_integer const& b);
bool operator!=(big_integer const& a, big_integer const& b);
bool operator<(big_integer const& a, big_integer const& b);
bool operator>(big_integer const& a, big_integer const& b);
bool operator<=(big_integer const& a, big_integer const& b);
bool operator>=(big_integer const& a, big_integer const& b);

std::string to_string(big_integer const& a);
std::ostream& operator<<(std::ostream& s, big_integer const& a);
