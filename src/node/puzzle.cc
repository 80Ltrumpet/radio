#include "puzzle.h"

#include "atomic.h"
#include "pickup.h"
#include "radio.h"
#include "scheduler.h"

class Tristate final {
 public:
  constexpr Tristate() : state_{State::Empty} {}
  constexpr Tristate(bool b) : state_{b ? State::True : State::False} {}

  operator bool() const { return state_ == State::True; }
  constexpr bool empty() const { return state_ == State::Empty; }

  constexpr bool operator==(const Tristate& other) const {
    return state_ == other.state_;
  }

  constexpr bool operator!=(const Tristate& other) const {
    return state_ != other.state_;
  }

 private:
  enum class State {
    Empty,
    False,
    True,
  };

  State state_;
};

namespace {

TaskHandle task_{};
Radio::FifoBuffer fifo_buffer_{};
bool outstanding_irq_{};
Tristate pickup_{};
Tristate prev_pickup_{};  // Last pickup state sent
bool sending_{};

void on_packet_sent() {
  outstanding_irq_ = true;
  sending_ = false;

  // Immediately start listening after sending a packet.
  Radio::Listen();
  task_->start();
}

void on_payload_ready([[maybe_unused]] int8_t rssi) {
  outstanding_irq_ = true;
  // TODO: Delay sending, not receiving.
  task_->start(20);
}

void on_pickup(bool is_picked_up) {
  outstanding_irq_ = true;
  pickup_ = is_picked_up;
  task_->start();
}

void send_state_locked(AtomicLock& lock) {
  using Puzzle::Header;
  prev_pickup_ = pickup_;
  sending_ = true;
  lock.unlock();
  Header packet{pickup_ ? Header::kTypePickUp : Header::kTypePutDown};
  Radio::Send(0, &packet, sizeof(packet));
  lock.lock();
}

void send_state() {
  AtomicLock lock{};
  send_state_locked(lock);
}

void run() {
  AtomicLock lock{};
  outstanding_irq_ = false;
  // If we get a pickup event while sending the last one, don't interrupt it.
  // This is extremely unlikely.
  if (sending_) return;
  lock.unlock();

  if (!Radio::IsListening() && Radio::Receive(&fifo_buffer_)) {
    auto bytes{fifo_buffer_.cbytes()};
    auto rhdr{reinterpret_cast<const Radio::Header*>(bytes)};
    auto phdr{reinterpret_cast<const Puzzle::Header*>(bytes + sizeof(*rhdr))};

    switch (phdr->type) {
    case Puzzle::Header::kTypeGetState:
      send_state();
      break;
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

  lock.lock();
  if (!sending_) {
    if (pickup_ != prev_pickup_) {
      send_state_locked(lock);
    } else {
      lock.unlock();
      Radio::Listen();
      lock.lock();
    }
  }

  if (!outstanding_irq_) {
    task_->pause();
  }
}

}  // namespace

namespace Puzzle {

void Init() {
  task_ = Scheduler::AddTask({"puzzle", run, 20});
  Radio::SetClient({on_payload_ready, on_packet_sent});
  Pickup::SetListener({on_pickup});
}

}  // namespace Puzzle