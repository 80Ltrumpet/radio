#pragma once

#include <stdint.h>

namespace Puzzle {

struct Header {
  enum Type : uint8_t {
    kTypePutDown,
    kTypePickUp,
    kTypeGetState,
    kTypeLedControl,
  };

  Header(Type t) : type{t} {}

  Type type;
};
static_assert(sizeof(Header) == 1, "Puzzle::Header size is incorrect");

// TODO: Define LED control payload.

void Init();

}  // namespace Puzzle