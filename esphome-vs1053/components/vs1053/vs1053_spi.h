#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"

namespace esphome {
namespace vs1053 {

#define SPI_BASE spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING
class VS1053SlowSPI : public spi::SPIDevice<SPI_BASE, spi::DATA_RATE_200KHZ> {};
class VS1053FastSPI : public spi::SPIDevice<SPI_BASE, spi::DATA_RATE_4MHZ> {};

/// This component provides a SPI interface for the device that allows
/// to communicate using both 200KHz and 4MHz SPI frequencies.
///
/// The templated SPI structure in ESPHome does not allow for variable
/// frequencies. This wrapper contains two concrete SPI instances for the two
/// frequencies and delegates SPI requests to eiter one of these, depending
/// on the need for slow or fast communication.
class VS1053SPI : public Component {
 public:
  // Methods for initialization.
  void set_slow_spi(VS1053SlowSPI *spi) { this->slow_spi_ = spi; }
  void set_fast_spi(VS1053FastSPI *spi) { this->fast_spi_ = spi; }
  void setup() override;

  // Methods for controlling the frequency.
  void go_slow();
  void go_fast();
  bool is_fast() const;

  // Methods for communicating to the device.
  void enable() const;
  void disable() const;
  void write_byte(uint8_t value) const;
  void write_byte16(uint16_t value) const;
  uint8_t read_byte() const;

 protected:
  VS1053SlowSPI *slow_spi_;
  VS1053FastSPI *fast_spi_;
  bool fast_mode_{false};
};

}  // namespace vs1053
}  // namespace esphome
