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
}

void VS1053Component::dump_config() {
  ESP_LOGCONFIG(TAG, "VS1053:");
  LOG_PIN("  XCS Pin: ", this->xcs_pin_);
  LOG_PIN("  XDCS Pin: ", this->xdcs_pin_);
  LOG_PIN("  DREQ Pin: ", this->dreq_pin_);
}

void VS1053Component::loop() {
  if (this->state_ == VS1053_INIT) {
    ESP_LOGI(TAG, "Initializing device");
    this->xcs_pin_->digital_write(true);
    this->xdcs_pin_->digital_write(true);
    this->to_state_(VS1053_RESET_1);
  }
  else if (this->state_ == VS1053_RESET_1) {
    if (this->state_ms_passed_(100)) {
      ESP_LOGD(TAG, "XCS/XDCS both to low to trigger reset");
      this->xcs_pin_->digital_write(false);
      this->xdcs_pin_->digital_write(false);
      this->to_state_(VS1053_RESET_2);
    }
  }
  else if (this->state_ == VS1053_RESET_2) {
    if (this->state_ms_passed_(500)) {
      ESP_LOGD(TAG, "XCS/XDCS both to high to finish reset");
      this->xcs_pin_->digital_write(true);
      this->xdcs_pin_->digital_write(true);
      this->to_state_(VS1053_SETUP_1);
    }
  }
  else if (this->state_ == VS1053_SETUP_1) {
    if (this->state_ms_passed_(500)) {
      // The device starts in slow SPI mode.
      this->spi_->go_slow();

      // Some basic communication tests to see if SPI is working.
      if (!this->test_communication_()) {
        this->to_state_(VS1053_FAILED); 
        ESP_LOGE(TAG, "Device initialized failed");
        return;
      }

      // Soft reset and see if SCI_MODE is set to the expected default value.
      this->soft_reset_();
      ESP_LOGD(TAG, "Check default status of SCI_MODE");
      auto mode = this->read_register_(SCI_MODE);
      if (mode != SM_SDINEW) {
        ESP_LOGE(TAG, "SCI_MODE not SM_SDINEW after reset (value is %d)", mode);
        ESP_LOGE(TAG, "Device initialized failed");
        this->to_state_(VS1053_FAILED); 
        return;
      }

      // Setup the device audio.
      ESP_LOGD(TAG, "Turning on analog audio at 44.1kHz");
      this->write_register_(SCI_AUDATA, 44101);

      // Set device clock multiplier to 3x, making speeds up to 5MHz possible.
      // After this, we can safely use a SPI speed of 4MHz therefore.
      ESP_LOGD(TAG, "Configuring device to allow high speed SPI clock");
      this->write_register_(SCI_CLOCKF, 0x6000);
      this->spi_->go_fast();

      this->to_state_(VS1053_SETUP_2); 
    }
  }
  else if (this->state_ == VS1053_SETUP_2) {
    if (!this->data_request_ready_()) {
      return;
    }

    // Some basic communication tests to see if SPI is working.
    if (!this->test_communication_()) {
      this->to_state_(VS1053_FAILED); 
      ESP_LOGE(TAG, "Device initialized failed");
      return;
    }

    this->to_state_(VS1053_READY); 
    ESP_LOGI(TAG, "Device initialized successfully");
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
  // The device must have pulled the DREQ pin high at this point.
  if (this->dreq_pin_->digital_read() == false) {
    ESP_LOGE(TAG, "DREQ is not HIGH, device connected correctly?");
    return false;
  }
  // Now test if we can write and read data over the
  // bus without errors. In fast SPI mode, we can perform more
  // write operations in the same time.
  auto step_size = this->spi_->is_fast() ? 15 : 300;
  auto cycles = 0;
  auto failures = 0;
  for (int value = 0; value < 0xFFFF; value += step_size) {
    cycles++;
    this->write_register_(SCI_VOL, value);
    auto read1 = this->read_register_(SCI_VOL);
    auto read2 = this->read_register_(SCI_VOL);
    if (value != read1 || value != read2) {
      failures++;
      ESP_LOGE(TAG, "SPI test failure after %d cycles; wrote %d, read back %d and %d",
               cycles, value, read1, read2);
      // Limit the number of reported failures.
      if (failures == 10) {
        break;
      }
    }
  }
  if (failures == 0) {
    ESP_LOGD(TAG, "SPI communication successful during %d write/read cycles", cycles);
    return true;
  } else {
    return false;
  }
}

void VS1053Component::soft_reset_() {
  ESP_LOGD(TAG, "soft resetting the device");
  this->write_register_(SCI_MODE, SM_SDINEW | SM_RESET);
  delay(5);
  this->wait_for_data_request_();
}

void VS1053Component::write_register_(uint8_t reg, uint16_t value) {
  this->control_mode_on_();
  this->spi_->write_byte(VS1053_WRITE_OP);
  this->spi_->write_byte(reg);
  this->spi_->write_byte16(value);
  this->control_mode_off_();
  ESP_LOGVV(TAG, "write_register: 0x%02X: 0x%02X", reg, value);
}

uint16_t VS1053Component::read_register_(uint8_t reg) {
  this->control_mode_on_();
  this->spi_->write_byte(VS1053_READ_OP);
  this->spi_->write_byte(reg);
  uint16_t value = this->spi_->read_byte() << 8 | this->spi_->read_byte();
  this->control_mode_off_();
  ESP_LOGVV(TAG, "read_register: 0x%02X: 0x%02X", reg, value);
  return value;
}

void VS1053Component::control_mode_on_() {
  this->spi_->enable();
  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(false);
}

void VS1053Component::control_mode_off_() {
  this->spi_->disable();
  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(true);
}

void VS1053Component::data_mode_on_() {
  this->spi_->enable();
  this->xcs_pin_->digital_write(true);
  this->xdcs_pin_->digital_write(false);
}

void VS1053Component::data_mode_off_() {
  this->spi_->disable();
  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(true);
}

bool VS1053Component::data_request_ready_() const {
  return this->dreq_pin_->digital_read() == true;
}

void VS1053Component::wait_for_data_request_() const {
  while (!this->data_request_ready_()) {
    delay(1);
  }
}

}  // namespace vs1053
}  // namespace esphome