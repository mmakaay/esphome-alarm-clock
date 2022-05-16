#include "vs10xx_plugin.h"

namespace esphome {
namespace vs10xx_base {

VS10XXPlugin::VS10XXPlugin(VS10XXSPI *spi) : spi_(spi) {}

// Implementation based on example code provided by plugin manuals, e.g.
// https://www.vlsi.fi/fileadmin/software/VS10XX/dacpatch.pdf
// This code is able to translate the compressed plugin format
// into SPI register writes.
void VS10XXPlugin::apply() {
  size_t i = 0;
  auto plugin = this->plugin_data_();
  auto plugin_size = plugin.size();

  while (i < plugin_size) {
    uint8_t addr = plugin[i++];
    uint16_t n = plugin[i++];

    // Replication mode: write multiple samples of the same value.
    if (n & 0x8000U) {
      n = n & 0x7FFF;
      uint16_t value = plugin[i++];
      while (n--) {
        this->spi_->write_register(addr, value);
      }
    // Copy mode: write multiple values.
    } else {
      while (n--) {
        uint16_t value = plugin[i++];
        this->spi_->write_register(addr, value);
      }
    }
  }
}

}  // namespace vs10xx_base
}  // namespace esphome
