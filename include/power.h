#pragma once

namespace Power {

// Disables unused functions and lights the sleep indication LED.
void Init();

// Enter the Idle sleep mode.
void SleepIdle();

}