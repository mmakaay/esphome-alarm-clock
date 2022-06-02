#include "vs10xx_hal_vs1003.h"

namespace esphome {
namespace vs10xx {

uint8_t VS1003Chipset::get_chipset_version() { return 3; }

uint16_t VS1003Chipset::get_fast_clockf() { return 0x9800; }

}  // namespace vs10xx
}  // namespace esphome
