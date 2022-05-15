#include "vs10xx_spi.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vs10xx_base {

void VS10XXSPI::setup() {
  this->slow_spi_->spi_setup();
  this->fast_spi_->spi_setup();

  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(true);
}

void VS10XXSPI::log_config() {
  ESP_LOGCONFIG(this->tag_, "  XCS Pin: %s", this->xcs_pin_->dump_summary().c_str());
  ESP_LOGCONFIG(this->tag_, "  XDCS Pin: %s", this->xdcs_pin_->dump_summary().c_str());
}

void VS10XXSPI::go_slow() { this->fast_mode_ = false; }

void VS10XXSPI::go_fast() { this->fast_mode_ = true; }

bool VS10XXSPI::is_fast() const { return this->fast_mode_; }

void VS10XXSPI::write_register(uint8_t reg, uint16_t value) const {
  this->begin_command_transaction();
  this->write_byte_(2); // command: write
  this->write_byte_(reg);
  this->write_byte16_(value);
  this->end_transaction();
  ESP_LOGVV(this->tag_, "write_register: 0x%02X: 0x%02X", reg, value);
}

uint16_t VS10XXSPI::read_register(uint8_t reg) const {
  this->begin_command_transaction();
  this->write_byte_(3); // command: read
  this->write_byte_(reg);
  uint16_t value = this->read_byte_() << 8 | this->read_byte_();
  this->end_transaction();
  ESP_LOGVV(this->tag_, "read_register: 0x%02X: 0x%02X", reg, value);
  return value;
}

void VS10XXSPI::begin_command_transaction() const {
  this->enable_();
  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(false);
}

void VS10XXSPI::begin_data_transaction() const {
  this->enable_();
  this->xcs_pin_->digital_write(true);
  this->xdcs_pin_->digital_write(false);
}

void VS10XXSPI::end_transaction() const {
  this->disable_();
  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(true);
}

void VS10XXSPI::enable_() const {
  if (this->fast_mode_) {
    this->fast_spi_->enable();
  } else {
    this->slow_spi_->enable();
  }
}

void VS10XXSPI::disable_() const {
  if (this->fast_mode_) {
    this->fast_spi_->disable();
  } else {
    this->slow_spi_->disable();
  }
}

void VS10XXSPI::write_byte_(uint8_t value) const {
  if (this->fast_mode_) {
    this->fast_spi_->write_byte(value);
  } else {
    this->slow_spi_->write_byte(value);
  }
}

void VS10XXSPI::write_byte16_(uint16_t value) const {
  if (this->fast_mode_) {
    this->fast_spi_->write_byte16(value);
  } else {
    this->slow_spi_->write_byte16(value);
  }
}

uint8_t VS10XXSPI::read_byte_() const {
  if (this->fast_mode_) {
    return this->fast_spi_->read_byte();
  } else {
    return this->slow_spi_->read_byte();
  }
}

}  // namespace vs10xx_base
}  // namespace esphome
