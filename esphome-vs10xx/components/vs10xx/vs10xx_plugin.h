#pragma once

#include "vs10xx_hal.h"

#include <vector>

namespace esphome {
namespace vs10xx {

/// Used for building classes that can apply patches or plugin code
/// to a device.
///
/// Code that can be applied can be found on the VLSI site: http://www.vlsi.fi
/// The compressed plugin format (.plg) is used, as recommended.
class VS10XXPlugin {
 public:
  explicit VS10XXPlugin() = default;

  /// Provide a short description for the plugin. 
  virtual const char* description() const = 0;

  /// Load the plugin code into the device.
  bool load(VS10XXHAL *hal);

 protected:
  /// Provide the plugin code, in compressed plugin format.
  /// This code can be copied literally from a downloaded .plg file.
  virtual const std::vector<uint16_t> plugin_data_() = 0;
};

}  // namespace vs10xx
}  // namespace esphome
