#include "vs10xx.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vs10xx {

static const char *const TAG = "vs10xx";

const char* device_state_to_text(DeviceState state) {
  switch (state) {
    case DEVICE_RESET:
      return "Performing hard reset";
    case DEVICE_INIT_PHASE_1:
      return "Initializing, phase 1";
    case DEVICE_INIT_PHASE_2:
      return "Initializing, phase 2";
    case DEVICE_REPORT_FAILED:
      return "Reporting a device failure";
    case DEVICE_FAILED:
      return "Device has failed";
    case DEVICE_READY:
      return "Ready for use";
    default:
      return "Unknown state";
  }
}

const char* media_state_to_text(MediaState state) {
  switch (state) {
    case MEDIA_STOPPED:
      return "Not playing";
    case MEDIA_STARTING:
      return "Starting playback";
    case MEDIA_PLAYING:
      return "Playing audio file";
    case MEDIA_SWITCHING:
      return "Switching to new audio file";
    case MEDIA_STOPPING:
      return "Stopping playback";
    default:
      return "Unknown state";
  }
}

void VS10XX::dump_config() {
  ESP_LOGCONFIG(TAG, "VS10XX:");
  this->hal->log_config();
  if (this->plugins_.size() > 0) {
    ESP_LOGCONFIG(TAG, "  Plugins:");
    for (auto *plugin : this->plugins_) {
      ESP_LOGCONFIG(TAG, "    - %s", plugin->description());
    }
  }
}

void VS10XX::setup() {
  ESP_LOGCONFIG(TAG, "Setting up device");
  this->preferences_store_ = global_preferences->make_preference<VS10XXPreferences>(this->get_object_id_hash());
  this->restore_preferences_();
}

void VS10XX::loop() {
  // When the HAL has ran into an issue, then go into failure mode.
  if (this->device_state_ != DEVICE_FAILED && this->hal->has_failed()) {
    this->set_device_state_(DEVICE_REPORT_FAILED); 
  }

  switch (this->device_state_) {
    case DEVICE_RESET:
      if (this->hal->reset()) {
        this->set_device_state_(DEVICE_INIT_PHASE_1);
      }
      break;
    case DEVICE_INIT_PHASE_1:
      if (this->hal->go_slow() &&
          this->hal->test_communication() &&
          this->hal->verify_chipset() &&
          this->hal->soft_reset()) {
        this->set_device_state_(DEVICE_INIT_PHASE_2);
      }
      break;
    case DEVICE_INIT_PHASE_2:
      if (!this->hal->go_fast() || !this->hal->test_communication()) {
        return;
      }
      for (auto *plugin : this->plugins_) {
        ESP_LOGD(TAG, "Load plugin: %s", plugin->description());
        if (!plugin->load(this->hal)) {
          return;
        }
      }

      this->hal->turn_off_output();
      ESP_LOGD(TAG, "Turning on analog audio at 44.1kHz stereo");
      this->hal->write_register(SCI_AUDATA, 44101);

      // All is okay, the device can be used.
      this->set_device_state_(DEVICE_READY); 
      ESP_LOGI(TAG, "Device initialized successfully");
      break;
    case DEVICE_REPORT_FAILED:
      ESP_LOGE(TAG, "Device failed");
      this->set_device_state_(DEVICE_FAILED);
      break;
    case DEVICE_READY:
      this->handle_media_operations_();
      break;
    case DEVICE_FAILED:
      // NOOP
      break;
  }
}

void VS10XX::handle_media_operations_() {
  switch (this->media_state_) {
    case MEDIA_STOPPED:
      // NOOP
      break;
    case MEDIA_STARTING:
      // XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
      break;
    default:
      // NOOP
      break;
  }
}

void VS10XX::set_device_state_(DeviceState state) {
  this->device_state_ = state;
  ESP_LOGD(TAG, "Device state: [%d] %s", state, device_state_to_text(state));
}

void VS10XX::set_media_state_(MediaState state) {
  this->media_state_ = state;
  ESP_LOGD(TAG, "Media state: [%d] %s", state, media_state_to_text(state));
}

void VS10XX::play(blob::Blob *blob) {
  if (this->device_state_ != DEVICE_READY) {
    ESP_LOGE(TAG, "play(): Device not ready (current state: %s)", device_state_to_text(this->device_state_));
  } else if (this->media_state_ == MEDIA_STOPPED) {
    ESP_LOGD(TAG, "play(): starting playback");
    this->set_media_state_(MEDIA_STARTING);
    this->audio_ = blob;
 } else if (this->media_state_ == MEDIA_PLAYING) {
    ESP_LOGD(TAG, "play(): Already playing, first stopping active playback");
    this->set_media_state_(MEDIA_SWITCHING);
    this->next_audio_ = blob;
  } else {
    ESP_LOGE(TAG, "play(): Current media state (%s) not supported play command", media_state_to_text(this->media_state_));
  }
}

void VS10XX::stop() {
  //if (this->device_state_ == DEVICE_READY) {
  //  ESP_LOGD(TAG, "stop(): The device was not playing, OK");
  //  return;
  //}
  //if (this->device_state_ == DEVICE_READY) {
  //}
  //if (this->device_state_ != DEVICE_PLAYING) {
  //  ESP_LOGE(TAG, "stop(): Cannot stop, the device is not playing audio");
  //  return;
  //}
  //this->audio_ = nullptr;
}

void VS10XX::store_preferences_() {
  this->preferences_store_.save(&this->preferences_); 
  ESP_LOGD(TAG, "Preferences stored");
}

void VS10XX::restore_preferences_() {
  if (!this->preferences_store_.load(&this->preferences_)) {
    ESP_LOGW(TAG, "Restoring preferences failed, using defaults");
    this->set_default_preferences_();
  } else {
    ESP_LOGD(TAG, "Preferences restored");
  }
  ESP_LOGD(TAG, "  - Volume left  : %0.2f", this->preferences_.volume_left);
  ESP_LOGD(TAG, "  - Volume right : %0.2f", this->preferences_.volume_right);
  ESP_LOGD(TAG, "  - Muted        : %s", YESNO(this->preferences_.muted));
}

void VS10XX::set_default_preferences_() {
  this->preferences_.volume_left = 1.0f;
  this->preferences_.volume_right = 1.0f;
  this->preferences_.muted = false;
}

uint32_t VS10XX::hash_base() { return 1527366430UL; }

}  // namespace vs10xx
}  // namespace esphome
