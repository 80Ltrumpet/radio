#pragma once

#include <stdint.h>

namespace Timer {

void Init();

uint64_t Millis();
uint64_t Micros();

void DelayMs(uint64_t ms);
void DelayUs(uint32_t us);

}