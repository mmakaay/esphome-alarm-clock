#include "vs10xx_base.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace vs10xx_base {

VS10XXBase::VS10XXBase(const char* tag, const Chipset supported_chipset) : tag_(tag), supported_chipset_version_(supported_chipset) {}

void VS10XXBase::setup() {
  ESP_LOGCONFIG(this->tag_, "Setting up device");
  this->hal_->set_tag(this->tag_);
}

void VS10XXBase::dump_config() {
  this->hal_->log_config();
}

void VS10XXBase::loop() {
  // When the HAL has detected an issue, then go into failure mode.
  if (this->state_ != VS10XX_FAILED && this->hal_->has_failed()) {
    this->to_state_(VS10XX_REPORT_FAILED); 
  }

  switch (this->state_) {
    case VS10XX_RESET:
      if (this->hal_->reset()) {
        this->to_state_(VS10XX_INIT_PHASE_1);
      }
      break;
    case VS10XX_INIT_PHASE_1:
      if (this->hal_->go_slow() &&
          this->test_communication_() &&
          this->hal_->verify_chipset(this->supported_chipset_version_) &&
          this->hal_->soft_reset()) {
        this->to_state_(VS10XX_INIT_PHASE_2); 
      }
      break;
    case VS10XX_INIT_PHASE_2:
      if (!this->hal_->go_fast() || !this->test_communication_()) {
        return;
      }
      for (auto *plugin : this->plugins_) {
        ESP_LOGD(this->tag_, "Load plugin: %s", plugin->description());
        if (!plugin->load(this->hal_)) {
          return;
        }
      }

      // Setup the device audio.
      this->turn_off_output();
      ESP_LOGD(this->tag_, "Turning on analog audio at 44.1kHz stereo");
      this->hal_->write_register(SCI_AUDATA, 44101);

      // All is okay, the device can be used.
      this->to_state_(VS10XX_READY); 
      ESP_LOGI(this->tag_, "Device initialized successfully");
      break;
    case VS10XX_REPORT_FAILED:
      ESP_LOGE(this->tag_, "Device initialization failed");
      this->to_state_(VS10XX_FAILED);
      break;
    case VS10XX_READY:
      // NOOP
      break;
    case VS10XX_FAILED:
      // NOOP
      break;
  }
}

void VS10XXBase::to_state_(State state) {
  this->state_ = state;
  this->state_timer_ = millis();
}

bool VS10XXBase::state_ms_passed_(uint32_t nr_of_ms) const {
  auto time_passed = millis() - this->state_timer_;
  return time_passed >= nr_of_ms;
}

bool VS10XXBase::test_communication_() {
  // Now test if we can write and read data over the
  // bus without errors. In fast SPI mode, we can perform more
  // write operations in the same time.
  auto step_size = this->hal_->is_fast() ? 30 : 300;
  auto cycles = 0;
  auto failures = 0;
  for (int value = 0; value < 0xFFFF; value += step_size) {
    cycles++;
    this->hal_->write_register(SCI_VOL, value);

    // Sanity check: DREQ should be LOW at this point. If not, then the
    // DREQ pin might not be connected correctly.
    if (cycles == 1) {
      if (this->hal_->is_ready()) {
        ESP_LOGE(this->tag_, "DREQ is unexpectedly HIGH after sending command");
        this->to_state_(VS10XX_REPORT_FAILED); 
        return false;
      }
      // Worst case, setting the volume could take 2100 clock cycles.
      // At 12.288Mhz, this is 171ms. After that, DREQ should be HIGH again.
      delay(200);
      if (!this->hal_->is_ready()) {
        ESP_LOGE(this->tag_, "DREQ is unexpectedly LOW after processing command");
        this->to_state_(VS10XX_REPORT_FAILED); 
        return false;
      }
    }

    auto read1 = this->hal_->read_register(SCI_VOL);
    auto read2 = this->hal_->read_register(SCI_VOL);
    if (value != read1 || value != read2) {
      failures++;
      ESP_LOGE(this->tag_, "SPI test failure after %d cycles; wrote %d, read back %d and %d",
               cycles, value, read1, read2);
      // Limit the number of reported failures.
      if (failures == 10) {
        break;
      }
    }
  }
  if (failures == 0) {
    ESP_LOGD(this->tag_, "SPI communication successful during %d write/read cycles", cycles);
    return true;
  } else {
    this->to_state_(VS10XX_REPORT_FAILED); 
    return false;
  }
}

void VS10XXBase::turn_off_output() {
  ESP_LOGD(this->tag_, "Turn off analog output");
  if (this->hal_->wait_for_ready()) {
    this->hal_->write_register(SCI_VOL, 0xFFFF);    
    this->hal_->wait_for_ready();
  }
}

void VS10XXBase::set_volume(VS10XXVolume vol) {
  ESP_LOGD(this->tag_, "Set output volume: left=%d, right=%d", vol.left, vol.right);
  if (this->hal_->wait_for_ready()) {
    auto left = clamp<uint8_t>(vol.left, 0, 30);
    auto right = clamp<uint8_t>(vol.right, 0, 30);
    // Translate 0 - 30 scale into 254 - 0 scale as used by the device.
    uint16_t left_ = (uint8_t)(254.0f - left * 254.0f/30.0f);
    uint16_t right_ = (uint8_t)(254.0f - right * 254.0f/30.0f);
    uint16_t value = (left_ << 8) | right_;
    this->hal_->write_register(SCI_VOL, value);
    this->hal_->wait_for_ready();
  }
  auto v = this->get_volume();
}

VS10XXVolume VS10XXBase::get_volume() const {
  uint16_t value = this->hal_->read_register(SCI_VOL);
  uint8_t left_ = (uint8_t)((value & 0xFF00) >> 8);
  uint8_t right_ = (uint8_t)(value & 0x00FF);
  uint8_t left = (int8_t)(30.0f - left_ * 30.0f/254.0f);
  uint8_t right = (int8_t)(30.0f - right_ * 30.0f/254.0f);
  return VS10XXVolume {
    left: left,
    right: right
  };
}

}  // namespace vs10xx_base
}  // namespace esphome
