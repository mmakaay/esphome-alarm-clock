#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/blob/blob.h"
#include "vs10xx_constants.h"
#include "vs10xx_hal.h"
#include "vs10xx_plugin.h"
#include <vector>

namespace esphome {
namespace vs10xx {

/// States used by the VS10XX code to implement its state machine. 
enum DeviceState {
  DEVICE_RESET,
  DEVICE_INIT_PHASE_1,
  DEVICE_INIT_PHASE_2,
  DEVICE_REPORT_FAILED,
  DEVICE_FAILED,
  DEVICE_READY,
};

enum MediaState {
  MEDIA_STOPPED,
  MEDIA_STARTING,
  MEDIA_PLAYING,
  MEDIA_STOPPING,
  MEDIA_SWITCHING,
};

class VS10XX : public Component {
 public:
  /// The hardware abstraction layer, used to talk to the hardware.
  VS10XXHAL *hal;

  // Object construction and configuration.
  explicit VS10XX();
  void set_hal(VS10XXHAL *hal) { this->hal = hal; }
  void add_plugin(VS10XXPlugin *plugin) { this->plugins_.push_back(plugin); }

  // These must be called by derived classes from their respective methods
  // when those are overridden.
  void setup() override;
  void dump_config() override;
  void loop() override;

  /// Play some audio.
  void play(blob::Blob *blob);

  /// Stop playing audio.
  void stop();

 protected:
  /// The name of this component.
  const char* name_;

  /// The VS10XX chipset that is supported by the implementation.
  const uint8_t supported_chipset_version_;

  /// The state of the device.
  DeviceState device_state_{DEVICE_RESET};

  /// The media operation state.
  MediaState media_state_{MEDIA_STOPPED};

  /// Plugins to load for this device.
  std::vector<VS10XXPlugin*> plugins_{};

  /// The Blob object from which audio must be played.
  blob::Blob *audio_{nullptr};
};

}  // namespace vs10xx
}  // namespace esphome
