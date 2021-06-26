#include "network.h"

#include <avr/pgmspace.h>
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
  // If it is equal to the broadcast address (or root address if not root),
  // call Radio::SetNodeAddress(Radio::kInvalidAddr).
  // The only way to transition out of this state is by running a "net addr ..."
  // command from the console.
  InvalidNodeAddress,
};

namespace {

constexpr uint8_t kRootAddr{0x00};

TaskHandle task_{};
State state_{State::Init};

void on_payload_ready() {
  // TODO
}

void on_packet_sent() {
  // TODO
}

void run() {
  // TODO
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

  static void SubcommandAddr(int argc, const char* argv[]);

 private:
  static const bool registered;
};

void NetworkCommand::CommandHandler(int argc, const char* argv[]) {
  if (argc < 2) {
    PrintUsage();
    return;
  }

  if (strcmp(argv[1], "addr") == 0) {
    SubcommandAddr(argc - 2, argv + 2);
  } else {
    PrintUsage();
  }
}

void NetworkCommand::PrintUsage() {
  // TODO: Add other subcommands.
  static PROGMEM const char* const kUsage{"Usage: net addr [<addr>]\n"};
  printf_P(kUsage);
}

void NetworkCommand::SubcommandAddr(int argc, const char* argv[]) {
  auto current_node_address{Radio::GetNodeAddress()};
  if (argc < 1) {
    printf("%02" PRIx8 "\n", current_node_address);
    return;
  }

#ifdef ANDRUIO_CONFIG_ROOT
  if (current_node_address != kRootAddr) {
    static PROGMEM const char* const kFmt{"Setting root address.\n"};
    printf_P(kFmt);
    Radio::SetNodeAddress(kRootAddr);
    // TODO: Restart the state machine.
  }
#else   // !ANDRUIO_CONFIG_ROOT
  // TODO: Disallow address changes if neighbor links are being torn down.

  errno = 0;
  auto addr{strtoul(*argv, nullptr, 16)};
  if (errno != 0 || addr > UINT8_MAX || addr == Radio::kInvalidAddr) {
    static PROGMEM const char* const kFmt{"Invalid hex node address \"%s\".\n"};
    printf_P(kFmt, *argv);
    return;
  }

  auto new_node_address{static_cast<uint8_t>(addr)};
  if (new_node_address == Radio::kBroadcastAddr ||
      new_node_address == kRootAddr) {
    static PROGMEM const char* const kFmt{
        "The node address must be distinct from the %s address.\n"};
    printf_P(kFmt, new_node_address == kRootAddr ? "root" : "broadcast");
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
    static PROGMEM const char* const kFmt{
        "Successfully set node address %02" PRIx8 ".\n"};
    printf_P(kFmt, new_node_address);
    // TODO: Restart the state machine (State::Init).
  }
#endif  // ANDRUIO_CONFIG_NODE
}

const char* const NetworkCommand::kCommandName{"net"};
const bool NetworkCommand::registered{
    CommandRegistry::RegisterCommand<NetworkCommand>()};
    