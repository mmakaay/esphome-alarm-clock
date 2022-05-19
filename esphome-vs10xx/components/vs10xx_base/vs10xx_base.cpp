#include "vs10xx_base.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vs10xx_base {

VS10XXBase::VS10XXBase(const char* name, const char* tag, const uint8_t supported_chipset) : name_(name), tag_(tag), supported_chipset_version_(supported_chipset) {}

void VS10XXBase::setup() {
  ESP_LOGCONFIG(this->tag_, "Setting up device");
  this->hal_->set_tag(this->tag_);
}

void VS10XXBase::dump_config() {
  ESP_LOGCONFIG(this->tag_, "%s:", this->name_);
  this->hal_->log_config();
  if (this->plugins_.size() > 0) {
    ESP_LOGCONFIG(this->tag_, "  Plugins:");
    for (auto *plugin : this->plugins_) {
      ESP_LOGCONFIG(this->tag_, "    - %s", plugin->description());
    }
  }
}

void VS10XXBase::loop() {
  // When the HAL has ran into an issue, then go into failure mode.
  if (this->state_ != VS10XX_FAILED && this->hal_->has_failed()) {
    this->state_ = VS10XX_REPORT_FAILED; 
  }

  switch (this->state_) {
    case VS10XX_RESET:
      if (this->hal_->reset()) {
        this->state_ = VS10XX_INIT_PHASE_1;
      }
      break;
    case VS10XX_INIT_PHASE_1:
      if (this->hal_->go_slow() &&
          this->hal_->test_communication() &&
          this->hal_->verify_chipset(this->supported_chipset_version_) &&
          this->hal_->soft_reset()) {
        this->state_ = VS10XX_INIT_PHASE_2;
      }
      break;
    case VS10XX_INIT_PHASE_2:
      if (!this->hal_->go_fast() || !this->hal_->test_communication()) {
        return;
      }
      for (auto *plugin : this->plugins_) {
        ESP_LOGD(this->tag_, "Load plugin: %s", plugin->description());
        if (!plugin->load(this->hal_)) {
          return;
        }
      }

      this->hal_->turn_off_output();
      ESP_LOGD(this->tag_, "Turning on analog audio at 44.1kHz stereo");
      this->hal_->write_register(SCI_AUDATA, 44101);

      // All is okay, the device can be used.
      this->state_ = VS10XX_READY; 
      ESP_LOGI(this->tag_, "Device initialized successfully");
      break;
    case VS10XX_REPORT_FAILED:
      ESP_LOGE(this->tag_, "Device initialization failed");
      this->state_ = VS10XX_FAILED;
      break;
    case VS10XX_READY:
      // NOOP
      break;
    case VS10XX_FAILED:
      // NOOP
      break;
  }
}

}  // namespace vs10xx_base
}  // namespace esphome
