#include "vs10xx.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vs10xx {

static const char *const TAG = "vs10xx";

VS10XX::VS10XX() : supported_chipset_version_(3) {}

void VS10XX::setup() {
  ESP_LOGCONFIG(TAG, "Setting up device");
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

void VS10XX::loop() {
  // When the HAL has ran into an issue, then go into failure mode.
  if (this->device_state_ != DEVICE_FAILED && this->hal->has_failed()) {
    this->device_state_ = DEVICE_REPORT_FAILED; 
  }

  switch (this->device_state_) {
    case DEVICE_RESET:
      if (this->hal->reset()) {
        this->device_state_ = DEVICE_INIT_PHASE_1;
      }
      break;
    case DEVICE_INIT_PHASE_1:
      if (this->hal->go_slow() &&
          this->hal->test_communication() &&
          this->hal->verify_chipset() &&
          this->hal->soft_reset()) {
        this->device_state_ = DEVICE_INIT_PHASE_2;
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
      this->device_state_ = DEVICE_READY; 
      ESP_LOGI(TAG, "Device initialized successfully");
      break;
    case DEVICE_REPORT_FAILED:
      ESP_LOGE(TAG, "Device failed");
      this->device_state_ = DEVICE_FAILED;
      break;
    case DEVICE_READY:
      // NOOP
      break;
    case DEVICE_FAILED:
      // NOOP
      break;
  }
}

void VS10XX::play(blob::Blob *blob) {
  if (this->device_state_ != DEVICE_READY) {
    ESP_LOGE(TAG, "play(): The device is not ready for use");
  } else if (this->media_state_ == MEDIA_STOPPED) {
    ESP_LOGD(TAG, "play(): start media");
    this->media_state_ = MEDIA_STARTING;
    this->audio_ = blob;
  } else if (this->media_state_ == MEDIA_PLAYING) {
    ESP_LOGD(TAG, "play(): Already playing, so need to stop first");
    this->media_state_ = MEDIA_SWITCHING;
    this->audio_ = blob;
  } else {
    ESP_LOGE(TAG, "play(): Current media state does not allow play command");
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

}  // namespace vs10xx
}  // namespace esphome
