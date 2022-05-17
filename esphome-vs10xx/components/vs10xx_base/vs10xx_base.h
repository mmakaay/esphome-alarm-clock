#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "vs10xx_constants.h"
#include "vs10xx_hal.h"
#include "vs10xx_plugin.h"
#include <vector>

namespace esphome {
namespace vs10xx_base {

enum State {
  VS10XX_RESET,
  VS10XX_INIT_PHASE_1,
  VS10XX_INIT_PHASE_2,
  VS10XX_REPORT_FAILED,
  VS10XX_FAILED,
  VS10XX_READY
};

struct VS10XXVolume {
  uint8_t left;
  uint8_t right;
};

class VS10XXBase : public Component {
 public:
  // Object construction and configuration.
  explicit VS10XXBase(const char* tag, const Chipset supported_chipset);
  void set_hal(VS10XXHAL *hal) { this->hal_ = hal; }
  void add_plugin(VS10XXPlugin *plugin) { this->plugins_.push_back(plugin); }

  // These must be called by derived classes from their respective methods.
  void setup() override;
  void dump_config() override;
  void loop() override;

  /// Turn off the analog output completely.
  void turn_off_output();

  /// Set the volume for the left and right analog output channels.
  /// The volume level range is 0 (silence) - 30 (max volume).
  void set_volume(VS10XXVolume volume);

  /// Get the volume for the left and right analog output channels.
  VS10XXVolume get_volume() const;

 protected:
  /// The tag to use for log messages.
  const char* tag_;

  /// The VS10XX chipset that is supported by the implementation.
  const Chipset supported_chipset_version_;

  /// The hardware abstraction layer, used to talk to the hardware.
  VS10XXHAL *hal_;

  /// Plugins to load for this device.
  std::vector<VS10XXPlugin*> plugins_{};

  /// Perform tests on the SPI communication to see if the bus is working.
  bool test_communication_();

  // Some utility fields and methods to implement a simple state machine. 
  State state_{VS10XX_RESET};
  uint32_t state_timer_; 
  void to_state_(State state);
  bool state_ms_passed_(uint32_t nr_of_ms) const;
};

}  // namespace vs10xx
}  // namespace esphome
