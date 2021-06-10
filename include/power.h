#pragma once

#include <stdint.h>

namespace Power {

// Disables unused functions and lights the sleep indication LED.
void Init();

// Called by the Timer module to allow Sleep to add milliseconds on wake-up.
void SetTimerMutator(void (*mutator)(uint16_t));

// Enter the Idle sleep mode with an optional millisecond timeout.
void Sleep(uint16_t sleep_ms = 0);

}