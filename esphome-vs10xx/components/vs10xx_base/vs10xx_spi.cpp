#include "vs10xx_spi.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vs10xx_base {

static const char *const TAG = "vs10xx_base";

void VS10XXSPI::setup() {
  ESP_LOGCONFIG(TAG, "Setting up VS10XXSPI");
  this->slow_spi_->spi_setup();
  this->fast_spi_->spi_setup();
}

void VS10XXSPI::go_slow() { this->fast_mode_ = false; }

void VS10XXSPI::go_fast() { this->fast_mode_ = true; }

bool VS10XXSPI::is_fast() const { return this->fast_mode_; }

void VS10XXSPI::write_byte(uint8_t value) const {
  if (this->fast_mode_) {
    this->fast_spi_->write_byte(value);
  } else {
    this->slow_spi_->write_byte(value);
  }
}

void VS10XXSPI::enable() const {
  if (this->fast_mode_) {
    this->fast_spi_->enable();
  } else {
    this->slow_spi_->enable();
  }
}

void VS10XXSPI::disable() const {
  if (this->fast_mode_) {
    this->fast_spi_->disable();
  } else {
    this->slow_spi_->disable();
  }
}

void VS10XXSPI::write_byte16(uint16_t value) const {
  if (this->fast_mode_) {
    this->fast_spi_->write_byte16(value);
  } else {
    this->slow_spi_->write_byte16(value);
  }
}

uint8_t VS10XXSPI::read_byte() const {
  if (this->fast_mode_) {
    return this->fast_spi_->read_byte();
  } else {
    return this->slow_spi_->read_byte();
  }
}

}  // namespace vs10xx_base
}  // namespace esphome
