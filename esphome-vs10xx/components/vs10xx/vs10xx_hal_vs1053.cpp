#include "vs10xx_hal_vs1053.h"

namespace esphome {
namespace vs10xx {

uint8_t VS1053Chipset::get_chipset_version() { return 4; };

uint16_t VS1053Chipset::get_fast_clockf() { return 0x9800; }

}  // namespace vs10xx
}  // namespace esphome
