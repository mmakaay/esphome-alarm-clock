#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "vs10xx_spi.h"
#include "vs10xx_plugin.h"
#include <vector>

namespace esphome {
namespace vs10xx_base {

/// The size of the data buffer on the device in bytes.
/// When streaming audio to the device, we must not send more than this
/// in one go.
const uint8_t VS10XX_CHUNK_SIZE = 32;

enum State {
  VS10XX_RESET,
  VS10XX_SETUP_SLOW_SPI,
  VS10XX_SETUP_FAST_SPI,
  VS10XX_REPORT_FAILED,
  VS10XX_FAILED,
  VS10XX_READY
};

/// Serial Command Interface (SCI) registers.
enum SCI_Register {
  SCI_MODE = 0,
  SCI_STATUS = 1,
  SCI_BASS = 2,
  SCI_CLOCKF = 3,
  SCI_DECODE_TIME = 4,
  SCI_AUDATA = 5,
  SCI_WRAM = 6,
  SCI_WRAMADDR = 7,
  SCI_AIADDR = 10,
  SCI_VOL = 11,
  SCI_AICTRL0 = 12,
  SCI_AICTRL1 = 13,
  SCI_NUM_REGISTERS = 15 
};

/// Serial Command Interface (SCI) mode bits that are used for setting the
/// SCI registers.
enum SCI_ModeBits {
  SM_DIFF = 1<<0,
  SM_LAYER12 = 1<<1,
  SM_RESET = 1<<2,
  SM_OUTOFWAV = 1<<3,
  SM_EARSPEAKER_LO = 1<<4,
  SM_TESTS = 1<<5,
  SM_STREAM = 1<<6,
  SM_EARSPEAKER_HI = 1<<7,
  SM_DACT = 1<<8,
  SM_SDIORD = 1<<9,
  SM_SDISHARE = 1<<10,
  SM_SDINEW = 1<<11,
  SM_ADPCM = 1<<12,
  SM_ADCPM_HP = 1<<13,
  SM_LINE_IN = 1<<14,
};

// Known chipset versions.
enum Chipset {
  CHIPSET_VS1001 = 0,
  CHIPSET_VS1011 = 1,
  CHIPSET_VS1002 = 2,
  CHIPSET_VS1003 = 3,
  CHIPSET_VS1053 = 4,
  CHIPSET_VS1033 = 5,
  CHIPSET_VS1103 = 6,
  CHIPSET_VS1063 = 7,
};

class VS10XXBase : public Component {
 public:
  // Object construction and configuration.
  explicit VS10XXBase(const char* tag, const Chipset supported_chipset);
  void set_dreq_pin(GPIOPin *dreq_pin) { this->dreq_pin_ = dreq_pin; }
  void set_reset_pin(GPIOPin *reset_pin) { this->reset_pin_ = reset_pin; }
  void set_spi(VS10XXSPI *spi) { this->spi_ = spi; }
  void add_plugin(VS10XXPlugin *plugin) { this->plugins_.push_back(plugin); }

  // These must be called by derived classes from their respective methods.
  void setup() override;
  void dump_config() override;
  void loop() override;

 protected:
  /// The tag to use for log messages.
  const char* tag_;

  /// The VS10XX chipset that is supported by the implementation.
  const Chipset supported_chipset_version_;

  /// The interface to the SPI bus.
  VS10XXSPI *spi_;

  /// The DREQ pin, which is used by the device to tell the MCU that
  /// it is open for business. This means: ready to process a command
  /// or to receive some audio data.
  GPIOPin *dreq_pin_;

  /// Optional reset pin. When this pin is linked to a GPIO (instead of the
  /// EN pin or Vcc for example), then the device can be turned on and off.
  /// Turning it off through the reset pin, offers the best power saving.
  GPIOPin *reset_pin_{nullptr};

  /// Plugins to load for this device.
  std::vector<VS10XXPlugin*> plugins_{};

  /// Trigger a hard reset.
  /// This will only work when a reset pin has been defined.
  /// A hard reset will reset all control registers and internal states
  /// of the device to their initial values. So after this, all software
  /// registers must be setup for decoding to work.
  void hard_reset_() const;

  /// Trigger a soft reset.
  /// This will reset the software decoder of the device.
  void soft_reset_() const;

  /// Returns the version of the VS10XX chipset.
  /// The vs10xx_base::Chipset enum contains the known chipset versions
  /// that the resported version can be compared against.
  uint8_t get_chipset_version_() const;

  /// Perform tests on the SPI communication to see if the bus is working.
  bool test_communication_() const;

  /// Wait for the device to become ready for action. 
  /// Note that this call is blocking. It would be better to modify this
  /// to make the waiting part of the loop code, so it doesn't block.
  void wait_for_data_request_() const;

  /// Checks if the device is ready for action.
  bool data_request_ready_() const;

  // SPI interaction methods.
  void control_mode_on_() const;
  void control_mode_off_() const;
  void data_mode_on_() const;
  void data_mode_off_() const;
  void write_register_(uint8_t reg, uint16_t value) const;
  uint16_t read_register_(uint8_t reg) const;

  // Some utility fields and methods to implement a simple state machine. 
  State state_{VS10XX_RESET};
  uint32_t state_timer_; 
  void to_state_(State state);
  bool state_ms_passed_(uint32_t nr_of_ms) const;

  // State handlers, called from the loop() method.
  void state_init_(); 
  void state_reset_(); 
  void state_setup_slow_spi_(); 
  void state_setup_fast_spi_(); 
  void state_report_failed_();
};

}  // namespace vs10xx
}  // namespace esphome
