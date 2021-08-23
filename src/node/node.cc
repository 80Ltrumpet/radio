#include "platform.h"

#include "pickup.h"
#include "usb.h"

namespace Platform {

void InitPreSei() {
  Usb::Init();
}

void InitPostSei() {
  Pickup::Init();
}

}  // namespace Platform