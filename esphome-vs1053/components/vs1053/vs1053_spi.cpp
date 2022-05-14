#include "vs1053_spi.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vs1053 {

static const char *const TAG = "vs1053";

void VS1053SPI::setup() {
  ESP_LOGCONFIG(TAG, "Setting up VS1053Component");
  this->slow_spi_->spi_setup();
  this->fast_spi_->spi_setup();
}

void VS1053SPI::go_slow() { this->fast_mode_ = false; }

void VS1053SPI::go_fast() { this->fast_mode_ = true; }

bool VS1053SPI::is_fast() const { return this->fast_mode_; }

void VS1053SPI::write_byte(uint8_t value) const {
  if (this->fast_mode_) {
    this->fast_spi_->write_byte(value);
  } else {
    this->slow_spi_->write_byte(value);
  }
}

void VS1053SPI::enable() const {
  if (this->fast_mode_) {
    this->fast_spi_->enable();
  } else {
    this->slow_spi_->enable();
  }
}

void VS1053SPI::disable() const {
  if (this->fast_mode_) {
    this->fast_spi_->disable();
  } else {
    this->slow_spi_->disable();
  }
}

void VS1053SPI::write_byte16(uint16_t value) const {
  if (this->fast_mode_) {
    this->fast_spi_->write_byte16(value);
  } else {
    this->slow_spi_->write_byte16(value);
  }
}

uint8_t VS1053SPI::read_byte() const {
  if (this->fast_mode_) {
    return this->fast_spi_->read_byte();
  } else {
    return this->slow_spi_->read_byte();
  }
}

}  // namespace vs1053
}  // namespace esphome
