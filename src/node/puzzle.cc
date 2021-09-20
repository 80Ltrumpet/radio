#include "puzzle.h"

#include "pickup.h"
#include "radio.h"
#include "scheduler.h"

namespace {

TaskHandle task_{};
Radio::FifoBuffer fifo_buffer_{};
bool pickup_{};

void on_packet_sent() {
  // Immediately start listening after sending a packet.
  Radio::Listen();
  task_->start();
}

void on_payload_ready([[maybe_unused]] int8_t rssi) {
  // TODO: Delay sending, not receiving.
  task_->start(50);
}

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
    auto rhdr{reinterpret_cast<const Radio::Header*>(bytes)};
    auto phdr{reinterpret_cast<const Puzzle::Header*>(bytes + sizeof(*rhdr))};

    switch (phdr->type) {
    case Puzzle::Header::kTypeGetState:
      send_state();
      return;
    case Puzzle::Header::kTypeLedControl: {
      auto led_ctrl{reinterpret_cast<const Puzzle::LedControlPacket*>(phdr)};
      // Broadcast LED control is for picked up nodes. Put down nodes should
      // be off.
      if (rhdr->dest == Radio::kAddrBroadcast && !pickup_) {
        Rgb::Clear();
        break;
      }

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
      break;
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