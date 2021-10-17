#pragma once

#include <stdint.h>

namespace Relay {

void Init();

void SwitchOn();
void SwitchOff(uint16_t delay_ms = 0);

}