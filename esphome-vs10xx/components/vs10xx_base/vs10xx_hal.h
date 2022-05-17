#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "vs10xx_constants.h"

namespace esphome {
namespace vs10xx_base {

// To communicate using both 200KHz and 4MHz SPI frequencies, two SPIDevice
// instances are used.
//
// The templated SPI structure in ESPHome does not allow for variable
// frequencies. This wrapper contains two concrete SPI instances for the two
// frequencies and delegates SPI requests to eiter one of these, depending
// on the need for slow or fast communication.
#define SPI_BASE spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING
class VS10XXSlowSPI : public spi::SPIDevice<SPI_BASE, spi::DATA_RATE_200KHZ> {};
class VS10XXFastSPI : public spi::SPIDevice<SPI_BASE, spi::DATA_RATE_4MHZ> {};

/// This component provides a hardware abstraction layer for VS10XX devices.
class VS10XXHAL : public Component {
 public:
  // Methods for initialization.
  explicit VS10XXHAL() = default;
  void set_tag(const char* tag) { this->tag_ = tag; }
  void set_slow_spi(VS10XXSlowSPI *spi) { this->slow_spi_ = spi; }
  void set_fast_spi(VS10XXFastSPI *spi) { this->fast_spi_ = spi; }
  void set_xdcs_pin(GPIOPin *xdcs_pin) { this->xdcs_pin_ = xdcs_pin; }
  void set_xcs_pin(GPIOPin *xcs_pin) { this->xcs_pin_ = xcs_pin; }
  void set_dreq_pin(GPIOPin *dreq_pin) { this->dreq_pin_ = dreq_pin; }
  void set_reset_pin(GPIOPin *reset_pin) { this->reset_pin_ = reset_pin; }
  void setup() override;
  void log_config();

  // Methods for controlling the SPI frequency.
  bool go_slow();
  bool go_fast();
  bool is_fast() const;

  /// Check if the device has failed.
  bool has_failed() const;

  /// Check if the device is ready for action.
  bool is_ready() const;

  /// Check if the device is ready for action within a given timeout.
  /// When the timeout (default 500ms) is reached after a delay during
  /// the provided process time (estimated time that the device will take
  /// to process the last command, default 0), without the device becoming
  /// ready, then false is returned and the device is registered as failed.
  /// Otherwise, true is returned.
  bool wait_for_ready(uint16_t process_time_ms=0, uint16_t timeout_ms=500);

  /// Check if a reset pin has been defined.
  bool has_reset() const;
  
  /// Hard reset the device. After this, it is recommended for the caller
  /// to wait for the device to become ready for the next data request.
  /// Returns true when the reset was succesful, false otherwise.
  /// The reason(s) for the failure will be logged as errors.
  bool reset();

  /// Soft reset the device.
  bool soft_reset();

  /// Check if the version of the VS10XX chipset matches the supported version.
  /// The vs10xx_base::Chipset enum contains the known chipset versions
  /// that the reported version can be compared against.
  bool verify_chipset(Chipset supported_version);

  // SPI interaction methods.
  bool write_register(uint8_t reg, uint16_t value);
  uint16_t read_register(uint8_t reg) const;
  void begin_command_transaction() const;
  void begin_data_transaction() const;
  void end_transaction() const;

 protected:
  /// The tag to use for log messages.
  const char* tag_;

  VS10XXSlowSPI *slow_spi_;
  VS10XXFastSPI *fast_spi_;
  bool fast_mode_{false};

  /// The XCS pin can be pulled low to lock the SPI bus for a command.
  GPIOPin *xcs_pin_;

  /// The XDCS pin can be pulled low to lock the SPI bus for a data transfer.
  GPIOPin *xdcs_pin_;

  /// The DREQ pin, which is used by the device to tell the MCU that
  /// it is open for business. This means: ready to process a command
  /// or to receive some audio data.
  GPIOPin *dreq_pin_;

  /// Optional reset pin. When this pin is linked to a GPIO (instead of the
  /// EN pin or Vcc for example), then the device can be turned on and off.
  /// Turning it off through the reset pin, offers the best power saving.
  GPIOPin *reset_pin_{nullptr};

  /// Whether or not a device error has been encountered.
  bool has_failed_{false};

  /// A little utility method that flags the device as failed and returns false.
  bool fail_();

  // Low level methods for communicating to one of the SPI device
  // instances (slow/fast).
  void enable_() const;
  void disable_() const;
  void write_byte_(uint8_t value) const;
  void write_byte16_(uint16_t value) const;
  uint8_t read_byte_() const;
};

}  // namespace vs10xx_base
}  // namespace esphome
