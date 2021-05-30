#pragma once

#include <stdint.h>

namespace Console {

// The device that provides console I/O must initialize the following before
// this function is called:
//  1. stdout/stderr (putc)
//  2. Console::SetGetChar(getc); [see below]
// Eventually, it might be prudent to avoid stdio.h altogether so that we only
// implement (and compile) what is needed.
void Init();

// Sets the console's internal "get_char" method. We avoid a stdin hook
// because commands are not interactive. That is, only the console directly
// accepts user input.
void SetGetChar(bool (*func)(char&));

}  // namespace Console