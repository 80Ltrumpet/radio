#include "puzzle.h"

#include "pickup.h"
#include "radio.h"
#include "scheduler.h"

namespace {

TaskHandle task_{};
Radio::FifoBuffer fifo_buffer_{};
bool pickup_{};

void on_packet_sent() { task_->start(); }

void on_payload_ready(int8_t rssi) { task_->start(); }

void on_pickup([[maybe_unused]] bool is_picked_up) { task_->start(); }

void send_state() {
  using Puzzle::Header;
  pickup_ = Pickup::IsPickedUp();
  Header packet{Pickup::IsPickedUp() ? Header::kTypePickUp
                                     : Header::kTypePutDown};
  Radio::Send(0, &packet, sizeof(packet));
}

void run() {
  task_->pause();

  if (Radio::Receive(&fifo_buffer_)) {
    auto bytes{fifo_buffer_.cbytes()};
    if (*bytes < sizeof(Radio::Header)) {
      // TODO: This is an ACK.
      Radio::Listen();
      return;
    }

    using Puzzle::Header;
    auto hdr{reinterpret_cast<const Header*>(bytes + sizeof(Radio::Header))};
    switch (hdr->type) {
      case Header::kTypeGetState:
        send_state();
        return;
      case Header::kTypeLedControl: {
        auto led_ctrl{reinterpret_cast<const Puzzle::LedControlPacket*>(hdr)};
        Rgb::Mutator rgb{};
        if (auto color{led_ctrl->get_color()}; color) {
          rgb.set_color(*color);
        }
        if (auto pattern{led_ctrl->get_pattern()}; pattern) {
          rgb.set_pattern(*pattern);
        }
        if (auto period_ms{led_ctrl->get_period()}; period_ms) {
          rgb.set_period(*period_ms);
        }
        if (auto transition_ms{led_ctrl->get_transition_period()};
            transition_ms) {
          rgb.set_transition_period(*transition_ms);
        }
        Radio::Send(0, nullptr, 0);
        return;
      }
    }
  }

  if (pickup_ != Pickup::IsPickedUp()) {
    send_state();
    return;
  }

  Radio::Listen();
}

}  // namespace

namespace Puzzle {

void Init() {
  task_ = Scheduler::AddTask({"puzzle", run});
  Radio::SetClient({on_payload_ready, on_packet_sent});
  Pickup::SetListener({on_pickup});
}

}  // namespace Puzzle