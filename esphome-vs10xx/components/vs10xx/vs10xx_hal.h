#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "vs10xx_constants.h"

namespace esphome {
namespace vs10xx {

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

/// This class holds status information for the device.
class VS10XXStatus {
 public:
  bool playing;
  AudioFormat format;

  explicit VS10XXStatus() { this->clear(); }

  void clear() {
    this->playing = false;
    this->format = FORMAT_UNKNOWN;
  }
};

/// This class describes the interface that must be implemented for
/// a HAL chipset. This interface contains all chipset-specific HAL code.
class VS10XXHALChipset {
 public:
  explicit VS10XXHALChipset() = default;

  /// Get the version of the chipset that is supported by the chipset class. 
  virtual uint8_t get_chipset_version() = 0;

  /// Get the SCI_CLOCKF value to use for fast (>4Mhz) communication.
  virtual uint16_t get_fast_clockf() = 0;
};

/// This component provides a hardware abstraction layer for VS10XX devices.
/// It encapsulates the communication and pin logic, and implements various
/// routines that can be performed by the implemented VS10XX chipsets. 
class VS10XXHAL : public Component {
 public:
  // Methods for initialization.
  explicit VS10XXHAL(VS10XXHALChipset *chipset) : chipset_(chipset) {}
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

  /// Check if the device is ready for action.
  bool is_ready() const;

  /// Check if the device is ready for action within a given timeout.
  /// When the timeout is reached, without the device becoming ready, then
  /// false is returned. Otherwise, true is returned.
  ///
  /// The default timeout is based on the worst case scenario from the
  /// VS1053 data sheet: 22000 XTALI / 12.288Mhz = 1.8ms.
  bool wait_for_ready(uint16_t timeout_ms=2);

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
  bool verify_chipset();

  /// Perform some communication tests to see if we can talk to the device.
  bool test_communication();

  /// Turn off the output.
  bool turn_off_output();

  /// Turn on audio output circuitry at 44.1kHz stereo.
  /// For now, I have not included options to set a different frequency,
  /// since this seems like a sane default to me.
  bool turn_on_output(); 

  /// Set the output volume of the left and right channel of the output.
  /// The volume goes from 0 (silent) to 30 (full volume). Out of bound values
  /// will be automatically clamped within these bounds.
  bool set_volume(float left, float right);

  /// Clear the decode time register, which tells us for how long the
  /// chip has been decoding audio.
  bool reset_decode_time();

  /// Retrieve the device status.
  VS10XXStatus& get_status();

  // High level SPI interaction methods.
  bool write_register(uint8_t reg, uint16_t value);
  uint16_t read_register(uint8_t reg) const;
  void begin_command_transaction() const;
  void begin_data_transaction() const;
  void end_transaction() const;

  // Low level SPI interaction methods.
  // These will switch between the slow and fast SPI instance, based on the
  // current frequency mode.
  void enable() const;
  void disable() const;
  void write_byte(uint8_t value) const;
  void write_byte16(uint16_t value) const;
  uint8_t read_byte() const;

 protected:
  VS10XXSlowSPI *slow_spi_;
  VS10XXFastSPI *fast_spi_;
  bool fast_mode_{false};

  /// This object implements the chipset-specific code.
  VS10XXHALChipset *chipset_;

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

  VS10XXStatus status_{};
};

}  // namespace vs10xx
}  // namespace esphome
