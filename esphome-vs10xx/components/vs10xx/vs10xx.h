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
  DEVICE_VERIFY_CHIPSET,
  DEVICE_SOFT_RESET,
  DEVICE_TO_FAST_SPI,
  DEVICE_LOAD_PLUGINS,
  DEVICE_INIT_AUDIO,
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
};

/// Translates a MediaState into a human readable text.
const char* media_state_to_text(MediaState state);

/// This struct holds the preferences for the device. These data are stored in
/// flash memory, so they can be restored after a device restart.
struct VS10XXPreferences {
  float volume_left{1.0f};
  float volume_right{1.0f};
  bool muted{false};
} __attribute__((packed));

/// Bitmask values that are used to keep track of what preferences need to be
/// sent to the device.
enum PreferencesChangeBits {
  CHANGE_NONE = 0x00,
  CHANGE_VOLUME = 0x01,
  CHANGE_MUTE = 0x02,         // TODO, example values for now
  CHANGE_EQUALIZER = 0x04,    // TODO, example values for now
  CHANGE_ALL = CHANGE_VOLUME  // TODO, update when mute and eq are added
};

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

  /// Set the output volume.
  /// The volume value must be between 0.0 (silent) and 1.0 (loud).
  void set_volume(float left, float right, bool publish = true);

  /// Change the output volume with a provided delta amount (-1.0 - 1.0).
  void change_volume(float delta);

  /// Play some audio.
  void play(blob::Blob *blob);

  /// Stop playing audio.
  void stop();

//  uint32_t hash_base() override;

 protected:
  // Members that handle device preferences. Setting preferences (e.g. the
  // volume) is handled asynchronously. When settings are updated, then it's
  // registered what settings have changed. Subsequently, the loop code will
  // try to sync the changed settings to the device.
  // The reason or the async behavior, is that it is never sure if the device
  // is ready to receive a command at any given time.
  ESPPreferenceObject preferences_store_;
  VS10XXPreferences preferences_{};
  void store_preferences_();
  void restore_preferences_();
  void set_default_preferences_();
  void sync_preferences_to_device_();
  uint8_t changed_preferences_{CHANGE_NONE};

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

  /// A buffer to store data that must be sent to the device.
  uint8_t buffer_[VS10XX_CHUNK_SIZE]{};

  /// The number of bytes in the buffer that must be sent to the device.
  size_t buffer_size_{0};

  HighFrequencyLoopRequester high_freq_;
};

}  // namespace vs10xx
}  // namespace esphome
