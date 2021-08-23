#pragma once

#include <stdint.h>

template <class T, uint8_t Size>
class RingBuffer final {
 public:
  bool full() const { return size_ == Size; }
  uint8_t size() const { return size_; }

  const T& operator[](uint8_t index) const {
    return buffer_[(head_ + index) % Size];
  }

  void push(const T& item) {
    if (size_ < Size) {
      buffer_[size_++] = item;
    } else {
      buffer_[head_] = item;
      head_ = (head_ + 1) % Size;
    }
  }

  void clear() {
    head_ = size_ = 0;
  }

 private:
  T buffer_[Size]{};
  uint8_t head_{};
  uint8_t size_{};
};