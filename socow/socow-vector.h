#pragma once
#include <cstddef>

namespace details {
template <typename T>
struct dynamic_storage {
  dynamic_storage() : capacity_(0), ref_count(1), data_(nullptr) {}

  dynamic_storage(size_t cap) : capacity_(cap), ref_count(1) {}

  size_t capacity_;
  size_t ref_count;
  T data_[0];
};
} // namespace details

template <typename T, size_t SMALL_SIZE>
struct socow_vector {
  using iterator = T*;
  using const_iterator = T const*;

  socow_vector() : size_(0), is_small(true) {}

  socow_vector(const socow_vector<T, SMALL_SIZE> &other)
      : socow_vector<T, SMALL_SIZE>() {
    size_ = other.size_;
    if (other.is_small) {
      copy(small_vector, other.small_vector, other.size_);
    } else {
      is_small = false;
      dynamic_vector = other.dynamic_vector;
      dynamic_vector->ref_count++;
    }
  }

  socow_vector<T, SMALL_SIZE>& operator=(const socow_vector<T, SMALL_SIZE>& other) {
    if (&other != this) {
      socow_vector<T, SMALL_SIZE>(other).swap(*this);
    }
    return *this;
  }

  ~socow_vector() {
    if (is_small)
      reset_data(small_vector, size_);
    else {
      cow_delete(dynamic_vector);
    }
  }

  T& operator[](size_t i) {
    return data()[i];
  }

  T const& operator[](size_t i) const {
    return data()[i];
  }

  const T* data() const {
    if (is_small) {
      return small_vector;
    }
    return dynamic_vector->data_;
  }

  T* data() {
    if (is_small) {
      return small_vector;
    }
    check_refs();
    return dynamic_vector->data_;
  }

  size_t size() const {
    return size_;
  }

  T& front() {
    return data()[0];
  }

  const T& front() const {
    return data()[0];
  }

  T& back() {
    return data()[size_ - 1];
  }

  const T& back() const {
    return data()[size_ - 1];
  }

  void push_back(const T& e) {
    if (size_ != capacity()) {
      new (data() + size_) T(e);
      size_++;
      return;
    }
    size_t new_cap = 2 * capacity() + 1;
    auto new_dyn_storage =
         create_new_buffer(new_cap, std::as_const(*this).data(), size_);

    try {
      new (new_dyn_storage->data_ + size_) T(e);
    } catch (...) {
      reset_data(new_dyn_storage->data_, size_);
      operator delete(new_dyn_storage);
      throw;
    }

    if (is_small) {
      reset_data(data(), size_);
    } else {
      cow_delete(dynamic_vector);
    }

    dynamic_vector = new_dyn_storage;
    is_small = false;
    size_++;
  }

  void pop_back() {
    data()[--size_].~T();
  }

  bool empty() const {
    return (size_ == 0);
  }

  size_t capacity() const {
    if (is_small)
      return SMALL_SIZE;
    return dynamic_vector->capacity_;
  }

  void reserve(size_t new_cap) {
    if (capacity() < new_cap) {
      set_capacity(new_cap);
    } else {
      check_refs();
    }
  }

  void shrink_to_fit() {
    if (is_small || size_ == dynamic_vector->capacity_) return;
    if (size_ <= SMALL_SIZE) {
      auto* tmp = dynamic_vector;
      dynamic_vector = nullptr;
      try {
        copy(small_vector, tmp->data_, size_);
      } catch (...) {
        dynamic_vector = tmp;
        throw;
      }
      cow_delete(tmp);
      is_small = true;
    } else {
      set_capacity(size_);
    }
  }

  void clear() {
    if (!is_small && dynamic_vector->ref_count > 1) {
      dynamic_vector->ref_count--;
      dynamic_vector = create_new_buffer(dynamic_vector->capacity_, nullptr, 0);
    } else {
      reset_data(data(), size_);
    }
    size_ = 0;
  }

  void swap(socow_vector& other) {
    if (is_small && !other.is_small) {
      swap_mixed(*this, other);
    } else if (!is_small && other.is_small) {
      swap_mixed(other, *this);
    } else if (is_small) {
      if (size_ >= other.size_) {
        swap_two_small(other, *this);
      } else {
        swap_two_small(*this, other);
      }
      std::swap(size_, other.size_);
    } else {
      std::swap(is_small, other.is_small);
      std::swap(size_, other.size_);
      std::swap(dynamic_vector, other.dynamic_vector);
    }
  }

  iterator begin() {
    return data();
  }

  iterator end() {
    return data() + size_;
  }

  const_iterator begin() const {
    return data();
  }

  const_iterator end() const {
    return data() + size_;
  }

  iterator insert(const_iterator pos, const T& e) {
    size_t index = pos - std::as_const(*this).begin();
    push_back(e);
    for (size_t i = size_ - 1; i > index; i--) {
      std::swap(data()[i - 1], data()[i]);
    }
    return begin() + index;
  }

  iterator erase(const_iterator pos) {
    return erase(pos, pos + 1);
  }

  iterator erase(const_iterator first, const_iterator last) {
    size_t first_index = first - std::as_const(*this).begin();
    size_t first_index_after_erase = first_index;
    size_t last_index = last - std::as_const(*this).begin();
    while (last_index != size_) {
      std::swap(data()[first_index], data()[last_index]);
      first_index++;
      last_index++;
    }
    for (size_t i = 0; i < last_index - first_index; i++) {
      pop_back();
    }
    return begin() + first_index_after_erase;
  }

private:

  void copy(T* dst, const T* src, size_t size, size_t from = 0) {
    for (size_t i = from; i != size; ++i) {
      try {
        new (dst + i) T(src[i]);
      } catch (...) {
        reset_data(dst, i);
        throw;
      }
    }
  }

  void reset_data(T* data, size_t size, size_t from = 0) {
    for (size_t i = size; i != from; i--) {
      data[i - 1].~T();
    }
  }

  void set_capacity(size_t new_cap) {
    auto new_dyn_storage =
        create_new_buffer(new_cap, std::as_const(*this).data(), size_);

    if (is_small) {
      reset_data(data(), size_);
    } else {
      cow_delete(dynamic_vector);
    }

    dynamic_vector = new_dyn_storage;
    is_small = false;
  }

  void cow_delete(details::dynamic_storage<T>* tmp) {
    if (tmp->ref_count > 1) {
      tmp->ref_count--;
    } else {
      reset_data(tmp->data_, size_);
      operator delete(tmp);
    }
  }

  void check_refs() {
    if (!is_small && dynamic_vector->ref_count > 1) {
      auto old_dynamic_vector = dynamic_vector;
      try {
        dynamic_vector = create_new_buffer(old_dynamic_vector->capacity_,
                                           old_dynamic_vector->data_, size_);
      } catch (...) {
        dynamic_vector = old_dynamic_vector;
        throw;
      }

      old_dynamic_vector->ref_count--;
    }
  }

  details::dynamic_storage<T>* create_new_buffer(size_t cap, const T* src, size_t size) {
    auto* tmp = static_cast<details::dynamic_storage<T>*>(operator new(
        sizeof(details::dynamic_storage<T>) +
        sizeof(T) * cap));
    auto new_buffer = (new (tmp) details::dynamic_storage<T>{cap});
    try {
      copy(new_buffer->data_, src, size);
    } catch (...) {
      operator delete(new_buffer);
      throw;
    }
    return new_buffer;
  }

  void swap_mixed(socow_vector<T, SMALL_SIZE>& small,
                  socow_vector<T, SMALL_SIZE>& big) {
    auto tmp = big.dynamic_vector;
    big.dynamic_vector = nullptr;
    try {
      copy(big.small_vector, small.small_vector, small.size_);
    } catch (...) {
      big.dynamic_vector = tmp;
      throw;
    }
    reset_data(small.small_vector, small.size_);

    small.dynamic_vector = tmp;
    std::swap(small.is_small, big.is_small);
    std::swap(small.size_, big.size_);
  }

  void swap_two_small(socow_vector<T, SMALL_SIZE>& small,
                      socow_vector<T, SMALL_SIZE>& big) {
    for (size_t i = 0; i < small.size_; i++) {
      std::swap(big.small_vector[i], small.small_vector[i]);
    }
    copy(small.small_vector, big.small_vector, big.size_, small.size_);
    reset_data(big.small_vector, big.size_, small.size_);
  }

private:
  bool is_small;
  size_t size_;
  union {
    details::dynamic_storage<T>* dynamic_vector;
    T small_vector[SMALL_SIZE];
  };
};
