#include "puzzle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_registry.h"
#include "eeprom.h"
#include "gpio.h"
#include "radio.h"
#include "scheduler.h"

enum class NodeState : uint8_t {
  Unknown,
  PutDown,
  PickedUp,
};

enum class PuzzleState : uint8_t {
  Initializing,
  Correct,
  Incorrect,
  Solved,
};

namespace {

Gpio relay_{PINE, 4};

TaskHandle task_{};
Radio::FifoBuffer fifo_buffer_{};

constexpr uint8_t kMaxNodes{Eeprom::Data::NodeOrder.length};
uint8_t node_count_{};
uint8_t node_order_[kMaxNodes];
// Index into node_order_
uint8_t node_expected_{};

NodeState node_state_[kMaxNodes];
PuzzleState puzzle_state_{PuzzleState::Initializing};

bool sent_get_state_{};

void on_packet_sent() {
  // Immediately start listening after sending a packet.
  Radio::Listen();

  if (sent_get_state_) {
    sent_get_state_ = false;
    // Wait for the response a bit before retrying.
    task_->start(200);
  } else {
    task_->start();
  }
}

void on_payload_ready([[maybe_unused]] int8_t rssi) {
  // TODO: Delay sending, not receiving.
  task_->start(50);
}

// Sends an LED control packet based on the current puzzle state. If node is
// zero, send a broadcast.
void update_leds(uint8_t node) {
  Puzzle::LedControlPacket led_ctrl{};
  if (node != 0 && node_state_[node - 1] == NodeState::PutDown) {
    led_ctrl.set_color({});
    led_ctrl.set_pattern(Rgb::Pattern::None);
    led_ctrl.set_transition_period(1000);
  } else {
    if (node == 0) node = Radio::kAddrBroadcast;
    switch (puzzle_state_) {
    case PuzzleState::Correct:
      led_ctrl.set_color({60, 0, 80});
      led_ctrl.set_pattern(Rgb::Pattern::SineWhite);
      led_ctrl.set_period(2000);
      led_ctrl.set_transition_period(1000);
      break;
    case PuzzleState::Incorrect:
    case PuzzleState::Initializing:
      led_ctrl.set_color({255, 0, 0});
      led_ctrl.set_pattern(Rgb::Pattern::SineOff);
      led_ctrl.set_period(500);
      led_ctrl.set_transition_period(500);
      break;
    case PuzzleState::Solved:
      led_ctrl.set_color({0, 255, 0});
      led_ctrl.set_pattern(Rgb::Pattern::Throb);
      led_ctrl.set_period(1000);
      led_ctrl.set_transition_period(500);
    }
  }

  Radio::Send(node, &led_ctrl, sizeof(led_ctrl));
}

void handle_pickup(uint8_t node) {
  const auto prev_puzzle_state{puzzle_state_};
  node_state_[node - 1] = NodeState::PickedUp;

  // Pick-up only affects the "correct" puzzle state.
  if (puzzle_state_ == PuzzleState::Correct) {
    if (node == node_order_[node_expected_]) {
      ++node_expected_;
      if (node_expected_ >= node_count_) {
        puzzle_state_ = PuzzleState::Solved;
        relay_.clear();
      }
    } else {
      puzzle_state_ = PuzzleState::Incorrect;
    }
  }

  update_leds(puzzle_state_ == prev_puzzle_state ? node : 0);
}

void handle_putdown(uint8_t node) {
  const auto prev_puzzle_state{puzzle_state_};
  node_state_[node - 1] = NodeState::PutDown;

  switch (puzzle_state_) {
  case PuzzleState::Correct:
    if (node_expected_ == 0) {
      // This should be impossible.
    } else if (node == node_order_[node_expected_ - 1]) {
      --node_expected_;
    } else {
      puzzle_state_ = PuzzleState::Incorrect;
    }
    break;
  case PuzzleState::Incorrect:
  case PuzzleState::Solved: {
    bool all_down{true};
    for (uint8_t i{}; i < node_count_; ++i) {
      if (const auto& state{node_state_[node_order_[i] - 1]};
          state != NodeState::PutDown) {
        all_down = false;
        break;
      }
    }

    if (all_down) {
      node_expected_ = 0;
      puzzle_state_ = PuzzleState::Correct;
      relay_.set();
    }
    break;
  }
  }

  update_leds(puzzle_state_ == prev_puzzle_state ? node : 0);
}

void run() {
  task_->pause();

  if (!Radio::IsListening() && Radio::Receive(&fifo_buffer_)) {
    auto bytes{fifo_buffer_.cbytes()};
    auto rhdr{reinterpret_cast<const Radio::Header*>(bytes)};
    auto phdr{reinterpret_cast<const Puzzle::Header*>(bytes + sizeof(*rhdr))};

    switch (phdr->type) {
    case Puzzle::Header::kTypePutDown:
      handle_putdown(rhdr->src);
      return;
    case Puzzle::Header::kTypePickUp:
      handle_pickup(rhdr->src);
      return;
    }
  }

  Radio::Listen();

  if (puzzle_state_ != PuzzleState::Initializing) return;

  // Check if there are any nodes in the unknown state.
  uint8_t unknown{};
  bool picked_up{};
  for (uint8_t i{}; i < node_count_; ++i) {
    const uint8_t node{node_order_[i]};
    const auto state{node_state_[node - 1]};
    if (state == NodeState::Unknown) {
      unknown = node;
      break;
    }
    if (state == NodeState::PickedUp) {
      picked_up = true;
    }
  }

  if (unknown) {
    // Ask for the state.
    sent_get_state_ = true;
    Puzzle::Header packet{Puzzle::Header::kTypeGetState};
    Radio::Send(unknown, &packet, sizeof(packet));
    return;
  }

  // Otherwise, we are no longer initializing.
  puzzle_state_ = picked_up ? PuzzleState::Incorrect : PuzzleState::Correct;
}

void pause() {
  Radio::Standby();
  task_->pause();
  node_count_ = 0;
}

void restart() {
  // The relay should be initially in its "normal" state (NO open; NC closed).
  relay_.set();
  relay_.out();

  // Parse the node order from EEPROM. All node addresses must be in
  // the range [1, kMaxNodes]. Initialize node states, too.
  Eeprom::Read(Eeprom::Data::NodeOrder, node_order_);
  node_count_ = 0;
  while (node_count_ < kMaxNodes && node_order_[node_count_] <= kMaxNodes) {
    node_state_[node_order_[node_count_++] - 1] = NodeState::Unknown;
  }

  if (node_count_ > 0) {
    puzzle_state_ = PuzzleState::Initializing;
    node_expected_ = 0;
    sent_get_state_ = false;

    task_->start();
  }
}

}  // namespace

namespace Puzzle {

void Init() {
  task_ = Scheduler::AddTask({"puzzle", run, 0, Task::kPause});
  Radio::SetClient({on_payload_ready, on_packet_sent});
  restart();
}

}  // namespace Puzzle

struct PuzzleCommand final {
  static void CommandHandler(int argc, const char* argv[]);
  static const char* const kCommandName;

 private:
  static const bool registered;
};

void PuzzleCommand::CommandHandler(int argc, const char* argv[]) {
  if (argc < 2) {
    goto usage;
  }

  if (strcmp(argv[1], "pause") == 0) {
    pause();
  } else if (strcmp(argv[1], "restart") == 0) {
    restart();
  } else if (strcmp(argv[1], "status") == 0) {
    if (node_count_ == 0) {
      puts("PAUSED");
    } else {
      printf("Nodes: ");
      for (uint8_t i{}; i < node_count_; ++i) {
        auto n{node_order_[i]};
        auto state{node_state_[n - 1]};
        char s = state == NodeState::PickedUp  ? '^'
                 : state == NodeState::PutDown ? '_'
                                               : '?';
        printf("%u%c%c", n, s, i == node_count_ - 1 ? '\n' : ' ');
      }
      switch (puzzle_state_) {
      case PuzzleState::Initializing:
        puts("INIT");
        break;
      case PuzzleState::Incorrect:
        puts("INCORRECT");
        break;
      case PuzzleState::Correct:
        puts("CORRECT");
        break;
      case PuzzleState::Solved:
        puts("SOLVED");
      }
    }
  } else if (strcmp(argv[1], "order") == 0) {
    uint8_t order[kMaxNodes];
    int i{};
    for (; i < argc - 2 && i < kMaxNodes; ++i) {
      auto addr{atoi(argv[i + 2])};
      if (addr < 1 || addr > kMaxNodes) {
        printf("Ordered addresses must be in the range [1, %u].\n", kMaxNodes);
        return;
      }
      order[i] = static_cast<uint8_t>(addr);
    }
    for (; i < kMaxNodes; ++i) {
      order[i] = Radio::kAddrInvalid;
    }
    Eeprom::Update(Eeprom::Data::NodeOrder, order);
  } else if (strcmp(argv[1], "toggle") == 0) {
    // Cheat code. :)
    relay_.toggle();
  } else {
usage:
    puts(
        "Usage: puzzle pause | restart | status\n"
        "              order ADDR...");
  }
}

const char* const PuzzleCommand::kCommandName{"puzzle"};
const bool PuzzleCommand::registered{
    CommandRegistry::RegisterCommand<PuzzleCommand>()};
