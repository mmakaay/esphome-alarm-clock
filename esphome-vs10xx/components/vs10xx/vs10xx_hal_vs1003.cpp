#include "vs10xx_hal_vs1003.h"

namespace esphome {
namespace vs10xx {

uint8_t VS1003Chipset::get_chipset_version() { return 3; }

uint16_t VS1003Chipset::get_fast_clockf() {
  // Recommended value from datasheet: "If typical values are wanted, the
  // Internal Clock Multiplier needs to be set to 3.0× after reset. Wait
  // until DREQ rises, then write value 0x9800 to SCI_CLOCKF"
  //
  // Clock frequency: XTALI=12.288 MHz
  // Clock multiplier: XTALI×3.0
  // Allowed multiplier addition: 1.5x
  return 0x9800;
}

}  // namespace vs10xx
}  // namespace esphome
