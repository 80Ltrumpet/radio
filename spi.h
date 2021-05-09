#ifndef _SPI_H
#define _SPI_H

#include <stddef.h>
#include <stdint.h>

void spi_init(void);
uint8_t spi_rw(uint8_t data);

#endif
