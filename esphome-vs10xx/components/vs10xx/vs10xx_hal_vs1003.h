#pragma once

#include "vs10xx_hal.h"

namespace esphome {
namespace vs10xx {

class VS1003Chipset : public VS10XXHALChipset {
  uint8_t get_chipset_version() override;
  uint16_t get_fast_clockf() override;
};

}  // namespace vs10xx
}  // namespace esphome
