#include "platform.h"

#include "mag.h"
#include "usb.h"

namespace Platform {

void InitPreSei() {
  Usb::Init();
}

void InitPostSei() {
  Mag::Init();
}

}  // namespace Platform