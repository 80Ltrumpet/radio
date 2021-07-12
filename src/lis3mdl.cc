#include "lis3mdl.h"

#include "lis3mdl_defs.h"
#include "twi.h"

namespace {

Twi::Device dev_{lis3mdl::kI2cAddr};

}

namespace lis3mdl {

using namespace Bits;

bool Exists() {
  uint8_t who_am_i{};
  dev_.read(Reg::WHO_AM_I, who_am_i);
  return who_am_i == Reg::Reset::WHO_AM_I;
}

void Configure() {
  uint8_t wbuf[]{
    OM_LP | DO_5HZ,
    FS_4GAUSS,
  };
  dev_.write(Reg::CTRL1, wbuf, sizeof(wbuf));
}

void Enable() {
  dev_.write(Reg::CTRL3, MD_CONTINUOUS);
}

void Disable() {
  dev_.write(Reg::CTRL3, MD_POWER_DOWN);
}

bool Available() {
  uint8_t status;
  dev_.read(Reg::STATUS, status);
  return status & ZYXDA;
}

Sample Poll() {
  Sample sample;
  dev_.read(Reg::OUT_X_L, &sample, sizeof(sample));
  return sample;
}

}