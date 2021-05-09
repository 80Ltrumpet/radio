#ifndef _SPI_H
#define _SPI_H

#include <stdint.h>

struct Spi final {
    static void Init();
    static uint8_t ReadWrite(uint8_t data);
};

#endif
