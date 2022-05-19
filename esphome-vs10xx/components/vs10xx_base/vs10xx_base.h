#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "vs10xx_constants.h"
#include "vs10xx_hal.h"
#include "vs10xx_plugin.h"
#include <vector>

namespace esphome {
namespace vs10xx_base {

/// States used by the VS10XXBase code to implement its state machine. 
enum State {
  VS10XX_RESET,
  VS10XX_INIT_PHASE_1,
  VS10XX_INIT_PHASE_2,
  VS10XX_REPORT_FAILED,
  VS10XX_FAILED,
  VS10XX_READY
};

class VS10XXBase : public Component {
 public:
  // Object construction and configuration.
  explicit VS10XXBase(const char* name, const char* tag, const uint8_t supported_chipset);
  void set_hal(VS10XXHAL *hal) { this->hal_ = hal; }
  VS10XXHAL* hal() { return this->hal_; }
  void add_plugin(VS10XXPlugin *plugin) { this->plugins_.push_back(plugin); }

  // These must be called by derived classes from their respective methods
  // when those are overridden.
  void setup() override;
  void dump_config() override;
  void loop() override;

 protected:
  /// The name of this component.
  const char* name_;

  /// The tag to use for log messages.
  const char* tag_;

  /// The VS10XX chipset that is supported by the implementation.
  const uint8_t supported_chipset_version_;

  /// The state of the device.
  State state_{VS10XX_RESET};

  /// The hardware abstraction layer, used to talk to the hardware.
  VS10XXHAL *hal_;

  /// Plugins to load for this device.
  std::vector<VS10XXPlugin*> plugins_{};
};

}  // namespace vs10xx
}  // namespace esphome
