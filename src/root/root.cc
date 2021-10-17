#include "platform.h"

#include "puzzle.h"
#include "relay.h"
#include "usart.h"

namespace Platform {

void InitPreSei() {
  Relay::Init();
  Usart::Init();
}

void InitPostSei() {
  Puzzle::Init();
}

}  // namespace Platform