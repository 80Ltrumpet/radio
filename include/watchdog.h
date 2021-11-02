#pragma once

namespace Watchdog {

void Init();

void Start();

// The "high priority" watchdog cannot be paused or kicked.
void StartHighPriority();
void StopHighPriority();

void Pause();
void Resume();

void Kick();

}