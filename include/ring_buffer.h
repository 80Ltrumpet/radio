#pragma once

#include <stdint.h>

template <class T, uint8_t Size>
class RingBuffer final {
 public:
  bool empty() const { return size_ == 0; }
  bool full() const { return size_ >= Size; }
  uint8_t size() const { return size_; }

  inline const T& operator[](uint8_t index) const {
    return at(index);
  }
  
  inline void clear() { head_ = size_ = 0; }

  // Inserts item at the back of the buffer, overwriting the head if full.
  void push_back(const T& item);

  // Retrieves and removes the item from the head of the buffer. If the buffer
  // is empty, this returns a default-initialized value.
  T pop_front();

 private:
  inline T& at(uint8_t index) {
    return buffer_[(head_ + index) % Size];
  }

  T buffer_[Size]{};
  uint8_t head_{};
  uint8_t size_{};
};

template <class T, uint8_t Size>
void RingBuffer<T, Size>::push_back(const T& item) {
  if (full()) {
    at(0) = item;
    ++head_ %= Size;
    // size_ stays the same.
  } else {
    at(size_++) = item;
  }
}

template <class T, uint8_t Size>
T RingBuffer<T, Size>::pop_front() {
  if (empty()) return T{};
  auto item{static_cast<T&&>(at(0))};
  ++head_ %= Size;
  --size_;
  return item;
}