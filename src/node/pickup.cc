#include "pickup.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>

#include "atomic.h"
#include "lsm6dsox.h"
#include "ring_buffer.h"
#include "scheduler.h"
#include "twi.h"

using namespace lsm6dsox;

enum class State : uint8_t {
  // Sleep state is inactive.
  Inactive,
  // IN/ACT is disabled to allow gyro to confirm pickup.
  OnlyGyro,
  // IN/ACT is enabled; sleep state is active; gyro is near baseline.
  ActiveGyro,
};

namespace {

constexpr auto kAccelFullScale{Bits::FS_XL_2G};
constexpr auto kAccelOdr{Bits::ODR_XL_26};

constexpr auto kGyroFullScale{Bits::FS_125};
constexpr auto kLsbPerDps{INT16_MAX / 125.0f};
constexpr auto kGyroOdr{Bits::ODR_G_208};
constexpr auto kGyroBdr{Bits::BDR_104 << Bits::BDR_GY_};
constexpr uint16_t kGyroBdrThreshold{12};

State state_{State::ActiveGyro};
Twi::Device imu_{kI2cAddr};
TaskHandle task_{};
Vec3<float> gyro_vec_{};
RingBuffer<Vec3<float>, 3> gyro_buffer_{};
bool outstanding_irq_{};
bool output_{};

void configure_imu() {
  uint8_t wbuf[4];

  // Soft reset.
  imu_.write(Reg::CTRL3_C, Reg::Reset::CTRL3_C | Bits::SW_RESET);
  // Disable I3C.
  imu_.write(Reg::CTRL9_XL, Reg::Reset::CTRL9_XL | Bits::I3C_DISABLE);
  // Configure FIFO for gyro (FIFO_CTRL3, FIFO_CTRL4).
  wbuf[0] = kGyroBdr;
  wbuf[1] = Bits::FIFO_MODE_STOP_ON_FULL;
  // Trigger COUNTER_BDR on every 12 gyro batches (COUNTER_BDR_REG1,
  // COUNTER_BDR_REG2).
  wbuf[2] = Bits::RST_COUNTER_BDR | Bits::TRIG_COUNTER_BDR;
  wbuf[2] |= (kGyroBdrThreshold >> 8) & Bits::CNT_BDR_TH_H;
  wbuf[3] = kGyroBdrThreshold & 0xff;
  imu_.write(Reg::FIFO_CTRL3, wbuf, 4);
  // Route COUNTER_BDR to INT2.
  imu_.write(Reg::INT2_CTRL, Bits::INT2_CNT_BDR);
  // Disable high-performance mode (CTRL6_C, CTRL7_G).
  wbuf[0] = Bits::XL_HM_MODE;
  wbuf[1] = Bits::G_HM_MODE;
  imu_.write(Reg::CTRL6_C, wbuf, 2);
  // Set the wake-up threshold to ~23.4 milli-g (WAKE_UP_THS, WAKE_UP_DUR).
  wbuf[0] = 3 & Bits::WK_THS;
  wbuf[1] = Bits::WAKE_THS_W;
  imu_.write(Reg::WAKE_UP_THS, wbuf, 2);
  // Route SLEEP_CHANGE to INT2.
  imu_.write(Reg::MD2_CFG, Bits::INT2_SLEEP_CHANGE);
  // Enable (in)activity detection.
  imu_.write(Reg::TAP_CFG2, Bits::INTERRUPTS_ENABLE | Bits::INACT_EN_G_SLEEP);
  // Enable the IMU (CTRL1_XL, CTRL2_G).
  wbuf[0] = kAccelOdr | kAccelFullScale;
  wbuf[1] = kGyroOdr | kGyroFullScale;
  imu_.write(Reg::CTRL1_XL, wbuf, 2);
}

inline constexpr float abs(float x) {
  return x >= 0.0f ? x : -x;
}

// Stores the accumulated sum of FIFO data in gyro_vec_.
void accumulate_gyro(const FifoData& data) {
  const auto& raw{data.data()};
  for (uint8_t i{}; i < 3; ++i) {
    gyro_vec_.v[i] += raw.v[i] / kLsbPerDps;
  }
}

// Stores the mean of a window of gyro data in gyro_vec_.
void get_gyro_mean() {
  gyro_vec_.x = gyro_vec_.y = gyro_vec_.z = 0.0f;
  const auto count{ProcessFifo(accumulate_gyro, kGyroBdrThreshold)};
  for (uint8_t i{}; i < 3; ++i) {
    gyro_vec_.v[i] /= count;
  }
}

void start_gyro_loop() {
  uint8_t wbuf[2];
  // Clear INTERRUPTS_ENABLE.
  imu_.write(Reg::TAP_CFG2, 0);
  // Start the FIFO and clear the BDR counter (FIFO_CTRL4, COUNTER_BDR_REG1).
  wbuf[0] = Bits::FIFO_MODE_STOP_ON_FULL;
  wbuf[1] = Bits::RST_COUNTER_BDR | Bits::TRIG_COUNTER_BDR;
  wbuf[1] |= (kGyroBdrThreshold >> 8) & Bits::CNT_BDR_TH_H;
  imu_.write(Reg::FIFO_CTRL4, wbuf, 2);
  // Clear the ring buffer.
  gyro_buffer_.clear();
}

void stop_gyro_loop() {
  // Stop the FIFO.
  imu_.write(Reg::FIFO_CTRL4, Bits::FIFO_MODE_BYPASS);
}

// Returns true if any of the axes of the last four window averages differ by
// more than the threshold.
bool is_gyro_twitching() {
  constexpr auto kGyroTwitchThreshold{0.05f};
  const auto length{gyro_buffer_.size()};
  for (uint8_t i{}; i < length; ++i) {
    const auto& v1{gyro_buffer_[i]};
    for (auto j{i + 1}; j < length; ++j) {
      const auto& v2{gyro_buffer_[j]};
      for (uint8_t a{}; a < 3; ++a) {
        if (abs(v1.v[a] - v2.v[a]) > kGyroTwitchThreshold) {
          return true;
        }
      }
    }
    for (uint8_t a{}; a < 3; ++a) {
      if (abs(v1.v[a] - gyro_vec_.v[a]) > kGyroTwitchThreshold) {
        return true;
      }
    }
  }
  return false;
}

void pick_up(bool is_picked_up) {
  if (output_ == is_picked_up) return;
  if (is_picked_up) {
    // TODO: Debug print, for now.
    puts("^");
  } else {
    // TODO: Debug print, for now.
    puts("v");
  }
  output_ = is_picked_up;
}

void run() {
  {
    AtomicLock lock{};
    outstanding_irq_ = false;
  }
  const bool asleep{imu_.read(Reg::WAKE_UP_SRC) & Bits::SLEEP_STATE};
  const bool fifo_ready{imu_.read(Reg::FIFO_STATUS2) & Bits::COUNTER_BDR_IA};

  switch (state_) {
    case State::Inactive:
      if (!asleep) {
        state_ = State::OnlyGyro;
        start_gyro_loop();
      }
      break;

    case State::OnlyGyro:
      if (fifo_ready) {
        get_gyro_mean();
        if (gyro_buffer_.full()) {
          if (is_gyro_twitching()) {
            pick_up(true);
          } else {
            state_ = State::ActiveGyro;
            imu_.write(Reg::TAP_CFG2,
                       Bits::INTERRUPTS_ENABLE | Bits::INACT_EN_G_SLEEP);
          }
        }
        gyro_buffer_.push(gyro_vec_);
      }
      break;

    case State::ActiveGyro:
      if (asleep) {
        pick_up(false);
        state_ = State::Inactive;
        stop_gyro_loop();
      } else if (fifo_ready) {
        get_gyro_mean();
        if (gyro_buffer_.full() && is_gyro_twitching()) {
          pick_up(true);
          state_ = State::OnlyGyro;
          // Clear INTERRUPTS_ENABLE.
          imu_.write(Reg::TAP_CFG2, 0);
        }
        gyro_buffer_.push(gyro_vec_);
      }
      break;
  }

  // Let the task be interrupt-driven.
  AtomicLock lock{};
  if (!outstanding_irq_) {
    task_->pause();
  }
}

}  // namespace

namespace Pickup {

void Init() {
  // Check if the IMU exists.
  if (imu_.read(Reg::WHO_AM_I) != Reg::Reset::WHO_AM_I) {
    return;
  }
  
  task_ = Scheduler::AddTask({"pickup", run, 0, Task::kPause});

  // Configure the external interrupt (rising edge).
  EICRA |= _BV(ISC31) | _BV(ISC30);
  EIMSK |= _BV(INT3);

  configure_imu();
}

}  // namespace Pickup

ISR(INT3_vect) {
  outstanding_irq_ = true;
  task_->start();
}