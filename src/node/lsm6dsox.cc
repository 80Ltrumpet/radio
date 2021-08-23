#include "lsm6dsox.h"

#include <alloca.h>

#include "twi.h"

namespace lsm6dsox {

uint16_t ProcessFifo(void (*visitor)(const FifoData&), uint16_t limit) {
  Twi::Device imu{kI2cAddr};
  uint16_t status;

  imu.read(Reg::FIFO_STATUS1, &status, sizeof(status));

  auto count{status & Bits::DIFF_FIFO};
  if (count == 0) {
    // The FIFO is empty.
    return 0;
  }

  if (limit != 0 && count > limit) {
    count = limit;
  }

  // Allocate enough stack space to read the FIFO.
  const auto alloc_size{count * sizeof(FifoData)};
  auto fifo{reinterpret_cast<FifoData*>(alloca(alloc_size))};

  // Visit every chunk of data in the FIFO.
  uint16_t processed{};
  do {
    auto size{count * sizeof(FifoData)};
    if (size > alloc_size) {
      size = alloc_size;
      count = size / sizeof(FifoData);
    }

    imu.read(Reg::FIFO_DATA_OUT_TAG, fifo, size);
    for (uint16_t i{}; i < count; ++i) {
      visitor(fifo[i]);
    }
    processed += count;

    imu.read(Reg::FIFO_STATUS1, &status, sizeof(status));
    count = status & Bits::DIFF_FIFO;
  } while (count > 0 && (limit == 0 || processed < limit));

  return processed;
}

}  // namespace lsm6dsox