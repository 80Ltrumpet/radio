#pragma once

namespace Pickup {

struct Listener final {
  operator bool() { return on_pickup != nullptr; }
  void (*on_pickup)(bool){};
};

void Init();

bool IsPickedUp();
void SetListener(Listener&& listener);

}  // namespace Pickup