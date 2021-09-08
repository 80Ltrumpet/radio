#include "platform.h"

#include "pickup.h"
#include "puzzle.h"
#include "rgb.h"
#include "usb.h"

namespace Platform {

void InitPreSei() {
  Usb::Init();
}

void InitPostSei() {
  Rgb::Init();
  Pickup::Init();
  Puzzle::Init();
}

}  // namespace Platform