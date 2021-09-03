#include "platform.h"

#include "puzzle.h"
#include "usart.h"

namespace Platform {

void InitPreSei() {
  Usart::Init();
}

void InitPostSei() {
  Puzzle::Init();
}

}  // namespace Platform