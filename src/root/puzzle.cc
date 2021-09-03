#include "puzzle.h"

#include <stdio.h>

#include "radio.h"
#include "scheduler.h"

namespace {

TaskHandle task_{};
Radio::FifoBuffer fifo_buffer_{};
int8_t rssi_{Radio::kRssiInvalid};

void on_packet_sent() {
  // STUB
}

void on_payload_ready(int8_t rssi) {
  rssi_ = rssi;
  task_->start();
}

void run() {
  if (Radio::Receive(&fifo_buffer_)) {
    const auto& bytes{fifo_buffer_.cbytes()};
    printf("%02x ", bytes[0]);
    for (uint8_t i{1}; i <= bytes[0]; ++i) {
      printf("%02x ", bytes[i]);
    }
    printf("(%d)\n", rssi_);
  }

  task_->pause();
  Radio::Listen(true);
}

}  // namespace

namespace Puzzle {

void Init() {
  task_ = Scheduler::AddTask({"puzzle", run});
  Radio::SetClient({on_payload_ready, on_packet_sent});
}

}  // namespace Puzzle