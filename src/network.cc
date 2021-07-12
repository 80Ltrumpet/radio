#include "network.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_registry.h"
#include "radio.h"
#include "scheduler.h"

enum class State {
  // Clear all data, get/check node address.
  Init,
  // Node address is invalid.
  // The only way to transition out of this state is by running a "net addr ..."
  // command from the console.
  InvalidNodeAddress,
  // DEBUG: For now...
  SendPacket,
  HandlePacket,
};

namespace {

constexpr uint8_t kRootAddr{0x00};

TaskHandle task_{};
State state_{State::Init};

// DEBUG
uint16_t count_{};  // Number of sent/received packets.
int8_t rssi_{0x7f};

void on_payload_ready(int8_t rssi) {
  // TODO
#if 1  // DEBUG
  rssi_ = rssi;
  task_->start();
#endif
}

void on_packet_sent() {
  // TODO
#if 1  // DEBUG
  ++count_;
#endif
}

// DEBUG
void packet_handler(const Radio::Packet& pkt) {
  uint16_t id{*reinterpret_cast<const uint16_t*>(pkt.payload)};
  printf("\rReceived packet %" PRIu16 " with %" PRId8 " dBm.\n", id, rssi_);
}

void run() {
  switch (state_) {
    case State::Init:
      // TODO: Clear all network state (in case of restart).
      // Check if we are ready to participate in the network.
#ifdef ANDRUIO_CONFIG_ROOT
      if (Radio::GetNodeAddress() != kRootAddr) {
        // There is only one root address.
        Radio::SetNodeAddress(kRootAddr);
#else  // ANDRUIO_CONFIG_NODE
      if (auto addr{Radio::GetNodeAddress()}; addr == Radio::kInvalidAddr ||
                                              addr == Radio::kBroadcastAddr ||
                                              addr == kRootAddr) {
        state_ = State::InvalidNodeAddress;
        task_->pause();
        return;
#endif
      }
      break;
    default:
      // TODO: All the other states...
      break;
  }
#if 1  // DEBUG
  switch (state_) {
    case State::Init:
#ifdef ANDRUIO_CONFIG_NODE
      Radio::SendPacket(kRootAddr, nullptr, 0);
      task_->set_period(3000);
      state_ = State::SendPacket;
#else  // ANDRUIO_CONFIG_ROOT
      Radio::Listen(true);
      task_->pause();
      state_ = State::HandlePacket;
#endif
      break;
    case State::SendPacket:
      Radio::SendPacket(kRootAddr, &count_, sizeof(count_));
      break;
    case State::HandlePacket:
      Radio::HandlePacket(packet_handler);
      Radio::Listen(true);
      [[fallthrough]];
    default:
      task_->pause();
      break;
  }
#endif
}

}  // namespace

namespace Network {

void Init() {
  task_ = Scheduler::AddTask({"network", run});
  Radio::SetEventHandler({on_payload_ready, on_packet_sent});
}

}  // namespace Network

/*------------------------------------------------------------------------------
 * Command
 */
class NetworkCommand final {
 public:
  static void CommandHandler(int argc, const char* argv[]);
  static const char* const kCommandName;

  static void PrintUsage();

#ifdef ANDRUIO_CONFIG_NODE
  static void SubcommandAddr(int argc, const char* argv[]);
#endif  // ANDRUIO_CONFIG_NODE

 private:
  static const bool registered;
};

void NetworkCommand::CommandHandler(int argc, const char* argv[]) {
  if (argc < 2) {
    PrintUsage();
    return;
  }

#ifdef ANDRUIO_CONFIG_NODE
  if (strcmp(argv[1], "addr") == 0) {
    SubcommandAddr(argc - 2, argv + 2);
  } else {
    PrintUsage();
  }
#endif  // ANDRUIO_CONFIG_NODE
}

void NetworkCommand::PrintUsage() {
  // TODO: Add other subcommands.
  printf("Usage: net addr [<addr>]\n");
}

#ifdef ANDRUIO_CONFIG_NODE
void NetworkCommand::SubcommandAddr(int argc, const char* argv[]) {
  auto current_node_address{Radio::GetNodeAddress()};
  if (argc < 1) {
    printf("%02" PRIx8 "\n", current_node_address);
    return;
  }

  // TODO: Disallow address changes if neighbor links are being torn down.

  errno = 0;
  auto addr{strtoul(*argv, nullptr, 16)};
  if (errno != 0 || addr > UINT8_MAX || addr == Radio::kInvalidAddr) {
    printf("Invalid hex node address \"%s\".\n", *argv);
    return;
  }

  auto new_node_address{static_cast<uint8_t>(addr)};
  if (new_node_address == Radio::kBroadcastAddr ||
      new_node_address == kRootAddr) {
    printf("The node address must be distinct from the %s address.\n",
           new_node_address == kRootAddr ? "root" : "broadcast");
    return;
  }

  if (new_node_address == current_node_address) {
    // There is nothing to do.
    return;
  }

  if (current_node_address != Radio::kInvalidAddr) {
    // TODO: Set aside the new address (don't reconfigure the radio, yet), tear
    // down all existing connections (neighbors), configure the radio with the
    // new address, and restart the state machine (State::Init).
  } else {
    Radio::SetNodeAddress(new_node_address);
    printf("Successfully set node address %02" PRIx8 ".\n", new_node_address);
    // TODO: Restart the state machine (State::Init).
  }
}
#endif  // ANDRUIO_CONFIG_NODE

const char* const NetworkCommand::kCommandName{"net"};
const bool NetworkCommand::registered{
    CommandRegistry::RegisterCommand<NetworkCommand>()};
