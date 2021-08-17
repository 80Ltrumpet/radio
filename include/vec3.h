#pragma once

template <class T>
struct Vec3 {
  Vec3() : x{}, y{}, z{} {}
  union {
    struct {
      T x;
      T y;
      T z;
    };
    T v[0];
  };
};
static_assert(sizeof(Vec3<char>) == 3, "Vec3 has incorrect size");