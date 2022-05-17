#include "vs10xx_hal.h"
#include "vs10xx_constants.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vs10xx_base {

void VS10XXHAL::setup() {
  this->xdcs_pin_->setup();
  this->xcs_pin_->setup();
  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(true);
  this->dreq_pin_->setup();
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(false);
  }
  this->slow_spi_->spi_setup();
  this->fast_spi_->spi_setup();
}

void VS10XXHAL::log_config() {
  ESP_LOGCONFIG(this->tag_, "  XCS Pin: %s", this->xcs_pin_->dump_summary().c_str());
  ESP_LOGCONFIG(this->tag_, "  XDCS Pin: %s", this->xdcs_pin_->dump_summary().c_str());
  ESP_LOGCONFIG(this->tag_, "  DREQ Pin: %s", this->dreq_pin_->dump_summary().c_str());
  if (this->reset_pin_ == nullptr) {
    ESP_LOGCONFIG(this->tag_, "  RESET Pin: N/A");
  } else {
    ESP_LOGCONFIG(this->tag_, "  RESET Pin: %s", this->reset_pin_->dump_summary().c_str());
  }
}

bool VS10XXHAL::has_reset() const { return this->reset_pin_ != nullptr; }

bool VS10XXHAL::reset() {
  // Sanity check: in case no reset pin has been defined, check if DREQ goes HIGH.
  if (!this->is_ready() && !this->has_reset()) {
    if (!this->wait_for_ready(0, 1000)) {
      ESP_LOGE(this->tag_, "DREQ not pulled HIGH by the device and no reset pin defined");
      ESP_LOGE(this->tag_, "Did you forget to pull up the reset pin, to boot the device?");
      return false;
    }
  }

  if (this->has_reset()) {
    ESP_LOGD(this->tag_, "Hard resetting the device");

    // By driving the XRESET-signal low, the device is reset.
    this->reset_pin_->digital_write(false);
    delay(1); // 1 ms delay is enough according to the specs
    this->reset_pin_->digital_write(true);

    // After initialization, the DREQ ought to be pulled HIGH.
    // The datasheet specifies max 50000 XTALI cycles for boot initialization.
    // At the default XTALI of 12.288 MHz, this takes about 4ms.
    // Therefore, 5ms ought to be enough for the device to become ready.
    if (!this->wait_for_ready(5)) {
      return false;
    }

    // Clear the fail state, since a hardware reset might have fixed an issue.
    this->has_failed_ = false;
  } else {
    ESP_LOGW(this->tag_, "Not performing hard reset, no reset pin defined"); 
  }

  // The device always starts in slow mode, so we'll have to follow pace.
  // When no reset pin is available, then it's still safe to talk slowly
  // to the SPI bus, since the device will follow our SPI clock signal.
  this->fast_mode_ = false;

  return true;
}

bool VS10XXHAL::soft_reset() {
  ESP_LOGD(this->tag_, "Soft resetting the device");
  
  // Turn on "NEW MODE", which means that the two SPI chip select pins XCS and
  // XDCS are controlled independently. These can be used to flag the device
  // that respectively the serial commmand interface (SCI) or serial data
  // interface (SDI) must be activated on the SPI bus.
  // In "SHARED MODE", only the XCS pin is used for controlling both SCI (when
  // the pin is pulled LOW) and SDI (when the pin is pulled HIGH). While this
  // saves a GPIO on the MCU, this prevents using the SPI bus for any other
  // SPI devices, since the device will always be listening for either commands
  // or data.
  this->write_register(SCI_MODE, SM_SDINEW | SM_RESET);

  // According to the specs, 2ms ought to be enough for the device to become
  // ready after the soft reset.
  if (!this->wait_for_ready(2)) {
    return false;
  }

  // Sanity check: see if SCI_MODE is now set to the expected value of SM_DSINEW.
  auto mode = this->read_register(SCI_MODE);
  if (mode != SM_SDINEW) {
    ESP_LOGE(this->tag_, "SCI_MODE not SM_SDINEW after reset (value is %d)", mode);
    return this->fail_();
  }

  return true;
}

bool VS10XXHAL::go_slow() {
  ESP_LOGD(this->tag_, "Configuring device for slow speed SPI communication");

  // Set device clock multiplier to the default of 1.0x. When using that setting,
  // the device can only use SPI on a low frequency setting.
  if (this->write_register(SCI_CLOCKF, 0x0000)) {
    this->fast_mode_ = false;
    return true;
  } else {
    return false;
  }
}

bool VS10XXHAL::go_fast() {
  ESP_LOGD(this->tag_, "Configuring device for high speed SPI communication");

  // Set device clock multiplier to the recommended value for typical use.
  // After this, we can safely use a SPI speed of 4MHz.
  // Note:
  // For VS1003 and VS1053, I see 0x9800 in both data sheets. They mean different things
  // for each chipset, but since the recommended value is the same, I haven't moved
  // this into chipset-specific code for now.
  if (this->write_register(SCI_CLOCKF, 0x9800)) {
    this->fast_mode_ = true;
    return true;
  } else {
    return false;
  }
}

bool VS10XXHAL::is_fast() const { return this->fast_mode_; }

bool VS10XXHAL::verify_chipset(Chipset supported_version) {
  // From the datasheet:
  // SCI_STATUS register has SS_VER in bits 4:7
  auto status = this->read_register(SCI_STATUS);
  auto version = (status & 0xf0) >> 4;

  if (version != supported_version) {
    ESP_LOGE(this->tag_, "Unsupported chipset version: %d (expected %d)",
             version, supported_version);
    return this->fail_();
  }
  ESP_LOGD(this->tag_, "Chipset version: %d, verified OK", version);
  return true;
}

bool VS10XXHAL::fail_() {
  this->has_failed_ = true;
  return false;
}

bool VS10XXHAL::has_failed() const {
  return this->has_failed_;
}

bool VS10XXHAL::is_ready() const {
  return this->dreq_pin_->digital_read() == true;
}

bool VS10XXHAL::wait_for_ready(uint16_t process_time_ms, uint16_t timeout_ms) {
  if (process_time_ms > 0) {
    delay(process_time_ms);
  }
  auto timeout_at = millis() + timeout_ms;
  while (!this->is_ready()) {
    if (millis() > timeout_at) {
      ESP_LOGE(this->tag_, "DREQ not HIGH within %dms timeout", timeout_ms);
      return this->fail_();
    }
    delay(1);
  }
  return true;
}

bool VS10XXHAL::write_register(uint8_t reg, uint16_t value) {
  if (this->wait_for_ready()) {
    this->begin_command_transaction();
    this->write_byte_(2); // command: write
    this->write_byte_(reg);
    this->write_byte16_(value);
    this->end_transaction();
    ESP_LOGVV(this->tag_, "write_register: 0x%02X: 0x%02X", reg, value);
    return true;
  } else {
    return false;
  }
}

uint16_t VS10XXHAL::read_register(uint8_t reg) const {
  this->begin_command_transaction();
  this->write_byte_(3); // command: read
  this->write_byte_(reg);
  uint16_t value = this->read_byte_() << 8 | this->read_byte_();
  this->end_transaction();
  ESP_LOGVV(this->tag_, "read_register: 0x%02X: 0x%02X", reg, value);
  return value;
}

void VS10XXHAL::begin_command_transaction() const {
  this->enable_();
  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(false);
}

void VS10XXHAL::begin_data_transaction() const {
  this->enable_();
  this->xcs_pin_->digital_write(true);
  this->xdcs_pin_->digital_write(false);
}

void VS10XXHAL::end_transaction() const {
  this->disable_();
  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(true);
}

void VS10XXHAL::enable_() const {
  if (this->fast_mode_) {
    this->fast_spi_->enable();
  } else {
    this->slow_spi_->enable();
  }
}

void VS10XXHAL::disable_() const {
  if (this->fast_mode_) {
    this->fast_spi_->disable();
  } else {
    this->slow_spi_->disable();
  }
}

void VS10XXHAL::write_byte_(uint8_t value) const {
  if (this->fast_mode_) {
    this->fast_spi_->write_byte(value);
  } else {
    this->slow_spi_->write_byte(value);
  }
}

void VS10XXHAL::write_byte16_(uint16_t value) const {
  if (this->fast_mode_) {
    this->fast_spi_->write_byte16(value);
  } else {
    this->slow_spi_->write_byte16(value);
  }
}

uint8_t VS10XXHAL::read_byte_() const {
  if (this->fast_mode_) {
    return this->fast_spi_->read_byte();
  } else {
    return this->slow_spi_->read_byte();
  }
}

}  // namespace vs10xx_base
}  // namespace esphome
