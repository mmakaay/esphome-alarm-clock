#include "vs10xx_base.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vs10xx_base {

VS10XXBase::VS10XXBase(const char* name, const char* tag, const uint8_t supported_chipset) : name_(name), tag_(tag), supported_chipset_version_(supported_chipset) {}

void VS10XXBase::setup() {
  ESP_LOGCONFIG(this->tag_, "Setting up device");
  this->hal->set_tag(this->tag_);
}

void VS10XXBase::dump_config() {
  ESP_LOGCONFIG(this->tag_, "%s:", this->name_);
  this->hal->log_config();
  if (this->plugins_.size() > 0) {
    ESP_LOGCONFIG(this->tag_, "  Plugins:");
    for (auto *plugin : this->plugins_) {
      ESP_LOGCONFIG(this->tag_, "    - %s", plugin->description());
    }
  }
}

void VS10XXBase::loop() {
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
          this->hal->verify_chipset(this->supported_chipset_version_) &&
          this->hal->soft_reset()) {
        this->device_state_ = DEVICE_INIT_PHASE_2;
      }
      break;
    case DEVICE_INIT_PHASE_2:
      if (!this->hal->go_fast() || !this->hal->test_communication()) {
        return;
      }
      for (auto *plugin : this->plugins_) {
        ESP_LOGD(this->tag_, "Load plugin: %s", plugin->description());
        if (!plugin->load(this->hal)) {
          return;
        }
      }

      this->hal->turn_off_output();
      ESP_LOGD(this->tag_, "Turning on analog audio at 44.1kHz stereo");
      this->hal->write_register(SCI_AUDATA, 44101);

      // All is okay, the device can be used.
      this->device_state_ = DEVICE_READY; 
      ESP_LOGI(this->tag_, "Device initialized successfully");
      break;
    case DEVICE_REPORT_FAILED:
      ESP_LOGE(this->tag_, "Device failed");
      this->device_state_ = DEVICE_FAILED;
      break;
    case DEVICE_READY:
      // NOOP
      break;
  }
}

void VS10XXBase::play(blob::Blob *blob) {
  if (this->device_state_ != DEVICE_READY) {
    ESP_LOGE(this->tag_, "play(): The device is not ready for use");
    return;
  }
  //if (this->media_state_ == DEVICE_PLAYING) {
  //  ESP_LOGD(this->tag_, "play(): Already playing, so need to stop first");
  //  this->media_state_ = MEDIA_STOPPING
  //}
  //if (this->device_state_ != DEVICE_READY) {
  //  ESP_LOGE(this->tag_, "play(): The device is not ready for playing audio");
  //  return;
  //}
  //ESP_LOGD(this->tag_, "play(): Start playing audio");
  //this->device_state_ = DEVICE_PLAYING;
  //this->audio_ = blob;
}

void VS10XXBase::stop() {
  //if (this->device_state_ == DEVICE_READY) {
  //  ESP_LOGD(this->tag_, "stop(): The device was not playing, OK");
  //  return;
  //}
  //if (this->device_state_ == DEVICE_READY) {
  //}
  //if (this->device_state_ != DEVICE_PLAYING) {
  //  ESP_LOGE(this->tag_, "stop(): Cannot stop, the device is not playing audio");
  //  return;
  //}
  //this->audio_ = nullptr;
}

}  // namespace vs10xx_base
}  // namespace esphome
