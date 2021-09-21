#pragma once

#include <avr/interrupt.h>
#include <avr/io.h>

// C++-friendly replacement for ATOMIC_BLOCK(ATOMIC_RESTORESTATE) that can be
// subclassed.
class AtomicLock {
 public:
  explicit AtomicLock() : sreg_{SREG} { cli(); }
  // Intentionally *not* declared virtual to avoid the need to define the
  // delete operator.
  ~AtomicLock() {
    if (locked_) SREG = sreg_;
  }

  AtomicLock(const AtomicLock&) = delete;
  AtomicLock& operator=(const AtomicLock&) = delete;

  AtomicLock(AtomicLock&&) = delete;
  AtomicLock& operator=(AtomicLock&&) = delete;

  void lock() {
    if (!locked_) {
      sreg_ = SREG;
      cli();
      locked_ = true;
      post_lock();
    }
  }

  void unlock() {
    if (locked_) {
      pre_unlock();
      locked_ = false;
      SREG = sreg_;
    }
  }

 protected:
  // Derived classes must call this in the constructor, if defined.
  virtual void post_lock() {}
  // Derived classes must call this in the destructor, if defined.
  virtual void pre_unlock() {}

 private:
  uint8_t sreg_;
  bool locked_{true};
};