#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "vs10xx_constants.h"
#include "vs10xx_hal.h"

namespace esphome {
namespace vs10xx {

static const char *const TAG = "vs10xx";

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
  ESP_LOGCONFIG(TAG, "  XCS Pin: %s", this->xcs_pin_->dump_summary().c_str());
  ESP_LOGCONFIG(TAG, "  XDCS Pin: %s", this->xdcs_pin_->dump_summary().c_str());
  ESP_LOGCONFIG(TAG, "  DREQ Pin: %s", this->dreq_pin_->dump_summary().c_str());
  if (this->reset_pin_ == nullptr) {
    ESP_LOGCONFIG(TAG, "  RESET Pin: N/A");
  } else {
    ESP_LOGCONFIG(TAG, "  RESET Pin: %s", this->reset_pin_->dump_summary().c_str());
  }
}

bool VS10XXHAL::has_reset() const { return this->reset_pin_ != nullptr; }

bool VS10XXHAL::reset() {
  // Sanity check: in case no reset pin has been defined, check if DREQ goes HIGH.
  if (!this->is_ready() && !this->has_reset()) {
    if (!this->wait_for_ready(0, 1000)) {
      ESP_LOGE(TAG, "DREQ not pulled HIGH by the device and no reset pin defined");
      ESP_LOGE(TAG, "Did you forget to pull up the reset pin, to boot the device?");
      return false;
    }
  }

  if (this->has_reset()) {
    ESP_LOGD(TAG, "Hard resetting the device");

    // By driving the XRESET-signal low, the device will reset.
    this->reset_pin_->digital_write(false);
    delay(1); // 1 ms delay is enough according to the specs
    this->reset_pin_->digital_write(true);

    // After initialization, the DREQ pin ought to be pulled HIGH.
    // The datasheet specifies max 50000 XTALI cycles for boot initialization.
    // At the default XTALI of 12.288 MHz, this takes about 4ms.
    // Therefore, 5ms ought to be enough for the device to become ready.
    if (!this->wait_for_ready(5)) {
      return false;
    }

    // Clear the fail state, since a hardware reset might have fixed an issue.
    this->has_failed_ = false;
  } else {
    ESP_LOGW(TAG, "Not performing hard reset, no reset pin defined"); 
  }

  // The device always starts in slow mode, so we'll have to follow pace.
  // When no reset pin is available, then it's still safe to talk slowly
  // to the SPI bus, since the device will follow our SPI clock signal.
  this->fast_mode_ = false;

  return true;
}

bool VS10XXHAL::soft_reset() {
  ESP_LOGD(TAG, "Soft resetting the device");
  
  // Turn on "NEW MODE", which means that the two SPI chip select pins XCS and
  // XDCS are controlled independently. These can be used to flag the device
  // that respectively the serial commmand interface (SCI) or serial data
  // interface (SDI) must be activated on the SPI bus.
  // In "SHARED MODE", only the XCS pin is used for controlling both SCI (when
  // the pin is pulled LOW) and SDI (when the pin is pulled HIGH). While this
  // saves a GPIO on the MCU, this prevents using the SPI bus for any other
  // SPI devices, since the device will always be listening for either commands
  // or data.
  // For now "NEW MODE" feels like the best choice for ESPHome, but if somebody
  // requires "SHARED MODE", then we can implement a config option for it.
  this->write_register(SCI_MODE, SM_SDINEW | SM_RESET);

  // According to the specs, 2ms ought to be enough for the device to become
  // ready after the soft reset.
  if (!this->wait_for_ready(2)) {
    return false;
  }

  // Sanity check: see if SCI_MODE is now set to the expected value of SM_DSINEW.
  auto mode = this->read_register(SCI_MODE);
  if (mode != SM_SDINEW) {
    ESP_LOGE(TAG, "SCI_MODE not SM_SDINEW after soft reset (value is %d)", mode);
    return this->fail_();
  }

  return true;
}

bool VS10XXHAL::go_slow() {
  ESP_LOGD(TAG, "Configuring device for slow speed SPI communication");

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
  ESP_LOGD(TAG, "Configuring device for high speed SPI communication");

  // Set device clock multiplier to the recommended value for typical use.
  // After this, we can safely use a SPI speed of 4MHz.
  if (this->write_register(SCI_CLOCKF, chipset_->get_fast_clockf)) {
    this->fast_mode_ = true;
    return true;
  } else {
    return false;
  }
}

bool VS10XXHAL::verify_chipset() {
  // From the datasheet:
  // SCI_STATUS register has SS_VER in bits 4:7
  auto status = this->read_register(SCI_STATUS);
  auto version = (status & 0xf0) >> 4;

  auto supported_version = chipset_->get_chipset_version();

  if (version != supported_version) {
    ESP_LOGE(TAG, "Unsupported chipset version: %d (expected %d)",
             version, supported_version);
    return this->fail_();
  }
  ESP_LOGD(TAG, "Chipset version: %d, verified OK", version);
  return true;
}

bool VS10XXHAL::test_communication() {
  // Wait for the device to become ready (DREQ high).
  if (!this->wait_for_ready()) {
    return false;
  }

  // Now test if we can write and read data over the
  // bus without errors. In fast SPI mode, we can perform more
  // write operations in the same time.
  auto step_size = this->fast_mode_ ? 30 : 300;
  auto cycles = 0;
  auto failures = 0;
  for (int value = 0; value < 0xFFFF; value += step_size) {
    cycles++;
    this->write_register(SCI_VOL, value);

    // Sanity check: DREQ should be LOW at this point. If not, then the
    // DREQ pin might not be connected correctly.
    if (cycles == 1) {
      if (this->is_ready()) {
        ESP_LOGE(TAG, "DREQ is unexpectedly HIGH after sending command");
        return this->fail_();
      }
      // Worst case, setting the volume could take 2100 clock cycles.
      // At 12.288Mhz, this is 171ms. After that, DREQ should be HIGH again.
      if (!this->wait_for_ready(200, 0)) {
        ESP_LOGE(TAG, "DREQ is unexpectedly LOW after processing command");
        return false;
      }
    }

    auto read1 = this->read_register(SCI_VOL);
    auto read2 = this->read_register(SCI_VOL);
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
  }
  return this->fail_();
}

bool VS10XXHAL::set_volume(Volume volume) {
  auto left = clamp<uint8_t>(volume.left, 0, 30);
  auto right = clamp<uint8_t>(volume.right, 0, 30);
  ESP_LOGD(TAG, "Set output volume: left=%d, right=%d", left, right);
  if (this->wait_for_ready()) {
    // Translate 0 - 30 scale into 254 - 0 scale as used by the device.
    uint16_t left_ = (uint8_t)(254.0f - left * 254.0f/30.0f);
    uint16_t right_ = (uint8_t)(254.0f - right * 254.0f/30.0f);
    uint16_t value = (left_ << 8) | right_;
    return this->write_register(SCI_VOL, value) && this->wait_for_ready();
  }
  return false;
}

Volume VS10XXHAL::get_volume() const {
  uint16_t value = this->read_register(SCI_VOL);
  uint8_t left_ = (uint8_t)((value & 0xFF00) >> 8);
  uint8_t right_ = (uint8_t)(value & 0x00FF);
  uint8_t left = (int8_t)(30.0f - left_ * 30.0f/254.0f);
  uint8_t right = (int8_t)(30.0f - right_ * 30.0f/254.0f);
  return Volume {
    left: left,
    right: right
  };
}

bool VS10XXHAL::turn_off_output() {
  ESP_LOGD(TAG, "Turn off analog output");
  return this->write_register(SCI_VOL, 0xFFFF) && this->wait_for_ready();
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
      ESP_LOGE(TAG, "DREQ not HIGH within %dms timeout", timeout_ms);
      return this->fail_();
    }
    delay(1);
  }
  return true;
}

bool VS10XXHAL::write_register(uint8_t reg, uint16_t value) {
  if (this->wait_for_ready()) {
    this->begin_command_transaction();
    this->write_byte(2); // command: write
    this->write_byte(reg);
    this->write_byte16(value);
    this->end_transaction();
    ESP_LOGVV(TAG, "write_register: 0x%02X: 0x%02X", reg, value);
    return true;
  } else {
    return false;
  }
}

uint16_t VS10XXHAL::read_register(uint8_t reg) const {
  this->begin_command_transaction();
  this->write_byte(3); // command: read
  this->write_byte(reg);
  uint16_t value = this->read_byte() << 8 | this->read_byte();
  this->end_transaction();
  ESP_LOGVV(TAG, "read_register: 0x%02X: 0x%02X", reg, value);
  return value;
}

void VS10XXHAL::begin_command_transaction() const {
  this->enable();
  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(false);
}

void VS10XXHAL::begin_data_transaction() const {
  this->enable();
  this->xcs_pin_->digital_write(true);
  this->xdcs_pin_->digital_write(false);
}

void VS10XXHAL::end_transaction() const {
  this->disable();
  this->xdcs_pin_->digital_write(true);
  this->xcs_pin_->digital_write(true);
}

void VS10XXHAL::enable() const {
  if (this->fast_mode_) {
    this->fast_spi_->enable();
  } else {
    this->slow_spi_->enable();
  }
}

void VS10XXHAL::disable() const {
  if (this->fast_mode_) {
    this->fast_spi_->disable();
  } else {
    this->slow_spi_->disable();
  }
}

void VS10XXHAL::write_byte(uint8_t value) const {
  if (this->fast_mode_) {
    this->fast_spi_->write_byte(value);
  } else {
    this->slow_spi_->write_byte(value);
  }
}

void VS10XXHAL::write_byte16(uint16_t value) const {
  if (this->fast_mode_) {
    this->fast_spi_->write_byte16(value);
  } else {
    this->slow_spi_->write_byte16(value);
  }
}

uint8_t VS10XXHAL::read_byte() const {
  if (this->fast_mode_) {
    return this->fast_spi_->read_byte();
  } else {
    return this->slow_spi_->read_byte();
  }
}

}  // namespace vs10xx
}  // namespace esphome
