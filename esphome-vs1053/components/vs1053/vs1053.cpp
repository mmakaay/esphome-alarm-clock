#include "vs1053.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vs1053 {

static const char *const TAG = "vs1053";

void VS1053Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up VS1053Component");
  this->dreq_pin_->setup();
  this->xdcs_pin_->setup();
  this->xcs_pin_->setup();
  this->spi_setup();
}

void VS1053Component::dump_config() {
  ESP_LOGCONFIG(TAG, "VS1053:");

  LOG_PIN("  XCS Pin: ", this->xcs_pin_);
  LOG_PIN("  XDCS Pin: ", this->xdcs_pin_);
  LOG_PIN("  DREQ Pin: ", this->dreq_pin_);
}

void VS1053Component::loop() {
  if (this->state_ == VS1053_INIT) {
    ESP_LOGCONFIG(TAG, "Initializing device");
    this->xcs_pin_->digital_write(true);
    this->xdcs_pin_->digital_write(true);
    this->to_state_(VS1053_RESET_1);
  }
  else if (this->state_ == VS1053_RESET_1) {
    if (this->state_ms_passed_(100)) {
      ESP_LOGCONFIG(TAG, "XCS/XDCS both to low to trigger reset");
      this->xcs_pin_->digital_write(false);
      this->xdcs_pin_->digital_write(false);
      this->to_state_(VS1053_RESET_2);
    }
  }
  else if (this->state_ == VS1053_RESET_2) {
    if (this->state_ms_passed_(500)) {
      ESP_LOGCONFIG(TAG, "XCS/XDCS both to high to finish reset");
      this->xcs_pin_->digital_write(true);
      this->xdcs_pin_->digital_write(true);
      this->to_state_(VS1053_SETUP_1);
    }
  }
  else if (this->state_ == VS1053_SETUP_1) {
    if (this->state_ms_passed_(500)) {
      if (this->test_communication_()) {
//        // Declick: disable analog.
//        this->write_register_(SCI_VOL, 0xFFFF);
//        this->write_register_(SCI_AUDATA, 10);
//        delay(100);
//        // Switch on analog parts.
//        this->write_register_(SCI_VOL, 0xFEFE);
//        this->write_register_(SCI_AUDATA, 44101); // 44.1kHz audio
//        this->write_register_(SCI_VOL, 0x2020);
        this->to_state_(VS1053_SETUP_2); 
      } else {
        this->to_state_(VS1053_FAILED); 
        ESP_LOGE(TAG, "Device initialized failed");
      }
    }
  }
  else if (this->state_ == VS1053_SETUP_2) {
    if (this->test_communication_()) {
      this->to_state_(VS1053_READY); 
      ESP_LOGI(TAG, "Device initialized and ready for use");
    } else {
      this->to_state_(VS1053_FAILED); 
      ESP_LOGE(TAG, "Device initialized failed");
    }
  }
}

void VS1053Component::to_state_(State state) {
  this->state_ = state;
  this->state_timer_ = millis();
}

bool VS1053Component::state_ms_passed_(uint32_t nr_of_ms) const {
  auto time_passed = millis() - this->state_timer_;
  return time_passed >= nr_of_ms;
}

bool VS1053Component::test_communication_() {
  // The device must have pulled the DREQ pin high.
  ESP_LOGCONFIG(TAG, "Checking DREQ status (should be HIGH)");
  if (this->dreq_pin_->digital_read() == false) {
    ESP_LOGE(TAG, "DREQ is not HIGH, device connected correctly?");
    return false;
  }
  // Now test if we can write and read data over the
  // bus without errors.
  ESP_LOGCONFIG(TAG, "Testing SPI communication");
  auto step_size = this->fast_mode_ ? 3 : 300;
  auto cycles = 0;
  for (int value = 0; value < 0xFFFF; value += step_size) {
    cycles++;
    this->write_register_(SCI_VOL, value);
    auto read1 = this->read_register_(SCI_VOL);
    auto read2 = this->read_register_(SCI_VOL);
    if (value != read1 || value != read2) {
      ESP_LOGE(TAG, "SPI Communication failure after %d cycles; wrote %d, read back %d and %d",
               cycles, value, read1, read2);
      return false;
    }
  }
  ESP_LOGCONFIG(TAG, "SPI communication successful during %d write/read cycles", cycles);
  return true;
}

void VS1053Component::write_register_(uint8_t reg, uint16_t value) {
  this->control_mode_on_();
  this->write_byte(VS1053_WRITE_OP);
  this->write_byte(reg);
  this->write_byte16(value);
  this->control_mode_off_();
  ESP_LOGVV(TAG, "write_register: 0x%02X: 0x%02X", reg, value);
}

uint16_t VS1053Component::read_register_(uint8_t reg) {
  this->control_mode_on_();
  this->write_byte(VS1053_READ_OP);
  this->write_byte(reg);
  uint16_t value = this->read_byte() << 8 | this->read_byte();
  this->control_mode_off_();
  ESP_LOGVV(TAG, "read_register: 0x%02X: 0x%02X", reg, value);
  return value;
}

void VS1053Component::control_mode_on_() {
  this->enable();
  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(false);
}

void VS1053Component::control_mode_off_() {
  this->disable();
  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(true);
}

void VS1053Component::data_mode_on_() {
  this->enable();
  this->xcs_pin_->digital_write(true);
  this->xdcs_pin_->digital_write(false);
}

void VS1053Component::data_mode_off_() {
  this->disable();
  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(true);
}

void VS1053Component::wait_for_data_request_() const {
  while (this->dreq_pin_->digital_read() == false) {
    delay(1);
  }
}

}  // namespace vs1053
}  // namespace esphome
