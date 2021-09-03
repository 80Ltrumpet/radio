#include "platform.h"

#include "pickup.h"
#include "puzzle.h"
#include "usb.h"

namespace Platform {

void InitPreSei() {
  Usb::Init();
}

void InitPostSei() {
  Pickup::Init();
  Puzzle::Init();
}

}  // namespace Platform