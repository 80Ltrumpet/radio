#include "puzzle.h"

#include <stdio.h>

#include "radio.h"
#include "scheduler.h"

namespace {

TaskHandle task_{};
Radio::FifoBuffer fifo_buffer_{};

void on_packet_sent() { task_->start(); }

void on_payload_ready([[maybe_unused]] int8_t rssi) { task_->start(); }

void run() {
  task_->pause();

  if (Radio::Receive(&fifo_buffer_)) {
    auto bytes{fifo_buffer_.cbytes()};
    auto rhdr{reinterpret_cast<const Radio::Header*>(bytes)};
    if (*bytes < sizeof(*rhdr)) {
      // TODO: This is an ACK.
      Radio::Listen(true);
      return;
    }

    auto phdr{reinterpret_cast<const Puzzle::Header*>(bytes + sizeof(*rhdr))};
    switch (phdr->type) {
      case Puzzle::Header::kTypePutDown: {
        Puzzle::LedControlPacket led_ctrl;
        led_ctrl.set_color({});
        led_ctrl.set_pattern(Rgb::Pattern::None);
        led_ctrl.set_transition_period(1000);
        Radio::Send(rhdr->src, &led_ctrl, sizeof(led_ctrl));
        return;
      }
      case Puzzle::Header::kTypePickUp: {
        Puzzle::LedControlPacket led_ctrl;
        led_ctrl.set_color({26, 230, 26});
        led_ctrl.set_pattern(Rgb::Pattern::Throb);
        led_ctrl.set_period(2000);
        led_ctrl.set_transition_period(400);
        Radio::Send(rhdr->src, &led_ctrl, sizeof(led_ctrl));
        return;
      }
      default:
        // Unexpected packet type
        break;
    }
  }

  Radio::Listen(true);
}

}  // namespace

namespace Puzzle {

void Init() {
  task_ = Scheduler::AddTask({"puzzle", run});
  Radio::SetClient({on_payload_ready, on_packet_sent});
}

}  // namespace Puzzle