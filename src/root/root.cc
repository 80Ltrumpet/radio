#include "platform.h"

#include "usart.h"

namespace Platform {

void InitPreSei() {
  Usart::Init();
}

void InitPostSei() {}

}  // namespace Platform