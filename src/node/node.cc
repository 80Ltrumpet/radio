#include "platform.h"

#include "imu.h"
#include "mag.h"
#include "usb.h"

namespace Platform {

void InitPreSei() {
  Usb::Init();
}

void InitPostSei() {
  Imu::Init();
  Mag::Init();
}

}  // namespace Platform