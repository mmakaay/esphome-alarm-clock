#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"

namespace esphome {
namespace vs10xx_base {

#define SPI_BASE spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING
class VS10XXSlowSPI : public spi::SPIDevice<SPI_BASE, spi::DATA_RATE_200KHZ> {};
class VS10XXFastSPI : public spi::SPIDevice<SPI_BASE, spi::DATA_RATE_4MHZ> {};

/// This component provides a SPI interface for the device that allows
/// to communicate using both 200KHz and 4MHz SPI frequencies.
///
/// The templated SPI structure in ESPHome does not allow for variable
/// frequencies. This wrapper contains two concrete SPI instances for the two
/// frequencies and delegates SPI requests to eiter one of these, depending
/// on the need for slow or fast communication.
class VS10XXSPI : public Component {
 public:
  // Methods for initialization.
  explicit VS10XXSPI() = default;
  void set_tag(const char* tag) { this->tag_ = tag; }
  void set_slow_spi(VS10XXSlowSPI *spi) { this->slow_spi_ = spi; }
  void set_fast_spi(VS10XXFastSPI *spi) { this->fast_spi_ = spi; }
  void set_xdcs_pin(GPIOPin *xdcs_pin) { this->xdcs_pin_ = xdcs_pin; }
  void set_xcs_pin(GPIOPin *xcs_pin) { this->xcs_pin_ = xcs_pin; }
  void setup() override;
  void log_config();

  // Methods for controlling the SPI frequency.
  void go_slow();
  void go_fast();
  bool is_fast() const;

  // SPI interaction methods.
  void write_register(uint8_t reg, uint16_t value) const;
  uint16_t read_register(uint8_t reg) const;
  void begin_command_transaction() const;
  void begin_data_transaction() const;
  void end_transaction() const;

 protected:
  /// The tag to use for log messages.
  const char* tag_;

  /// The XCS pin can be pulled low to lock the SPI bus for a command.
  GPIOPin *xcs_pin_;

  /// The XDCS pin can be pulled low to lock the SPI bus for a data transfer.
  GPIOPin *xdcs_pin_;

  VS10XXSlowSPI *slow_spi_;
  VS10XXFastSPI *fast_spi_;
  bool fast_mode_{false};

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
