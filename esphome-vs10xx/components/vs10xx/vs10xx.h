#pragma once

#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/preferences.h"
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

/// Translates a DeviceState into a human readable text.
const char* device_state_to_text(DeviceState state);

enum MediaState {
  MEDIA_STOPPED,
  MEDIA_STARTING,
  MEDIA_PLAYING,
  MEDIA_STOPPING,
  MEDIA_SWITCHING,
};

/// Translates a MediaState into a human readable text.
const char* media_state_to_text(MediaState state);

struct VS10XXPreferences {
  float volume_left{1.0f};
  float volume_right{1.0f};
  bool muted{false};
} __attribute__((packed));

class VS10XX : public EntityBase, public Component {
 public:
  /// The hardware abstraction layer, used to talk to the hardware.
  VS10XXHAL *hal;

  // Object construction and configuration.
  explicit VS10XX() = default;
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

  uint32_t hash_base() override;

 protected:
  // Preferences are stored in flash memory.
  ESPPreferenceObject preferences_store_;
  VS10XXPreferences preferences_{};
  void store_preferences_();
  void restore_preferences_();
  void set_default_preferences_();

  /// Plugins to load for this device.
  std::vector<VS10XXPlugin*> plugins_{};

  DeviceState device_state_{DEVICE_RESET};
  void set_device_state_(DeviceState state);

  MediaState media_state_{MEDIA_STOPPED};
  void set_media_state_(MediaState state);

  /// When the device is ready for use, then this method is responsible for
  /// handling media oparations.
  void handle_media_operations_();

  /// The Blob object from which audio must be played.
  blob::Blob *audio_{nullptr};

  /// The next Blob object from which to play audio.
  /// This is used in case play() is called while another
  /// audio file is being played.
  blob::Blob *next_audio_{nullptr};
};

}  // namespace vs10xx
}  // namespace esphome
