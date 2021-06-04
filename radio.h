#pragma once

#include "spi.h"

// Low-level radio communication API. Networking will be handled by a separate
// module (task).
namespace Radio {

void Init();

uint8_t GetNodeAddress();

}