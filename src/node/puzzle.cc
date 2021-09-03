#include "puzzle.h"

#include "pickup.h"
#include "radio.h"
#include "scheduler.h"

namespace {

TaskHandle task_{};

void on_packet_sent() {
  // STUB
}

void on_payload_ready(int8_t rssi) {
  // STUB
}

void on_pickup([[maybe_unused]] bool is_picked_up) { task_->start(); }

void run() {
  Puzzle::Header packet{Pickup::IsPickedUp() ? Puzzle::Header::kTypePickUp
                                             : Puzzle::Header::kTypePutDown};
  Radio::Send(0, &packet, sizeof(packet));
  task_->pause();
}

}  // namespace

namespace Puzzle {

void Init() {
  task_ = Scheduler::AddTask({"puzzle", run, 0, Task::kPause});
  Radio::SetClient({on_payload_ready, on_packet_sent});
  Pickup::SetListener({on_pickup});
}

}  // namespace Puzzle