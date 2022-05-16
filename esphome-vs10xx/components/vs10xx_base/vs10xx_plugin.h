#pragma once

#include "vs10xx_spi.h"

#include <vector>

namespace esphome {
namespace vs10xx_base {

/// Used for building classes that can apply patches or plugin code
/// to a device.
///
/// Code that can be applied can be found on the VLSI site: http://www.vlsi.fi
/// The compressed plugin format (.plg) is used, as recommended.
class VS10XXPlugin {
 public:
  explicit VS10XXPlugin(VS10XXSPI *spi);

  /// Apply the plugin code by sending it to the device.
  void apply();

 protected:
  /// The SPI interface of the device to patch.
  VS10XXSPI *spi_;

  /// Return the log tag to use for this plugin.
  virtual const char* tag_() const  = 0;

  /// Return the plugin code, in compressed plugin format.
  /// This code can be copied literally from a downloaded .plg file.
  virtual const std::vector<uint16_t> plugin_data_() = 0;
};

}  // namespace vs10xx
}  // namespace esphome
