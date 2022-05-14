#pragma once

#include "vs1053_spi.h"
#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"

namespace esphome {
namespace vs1053 {

enum State {
  VS1053_INIT,
  VS1053_RESET_1,
  VS1053_RESET_2,
  VS1053_SETUP_1,
  VS1053_SETUP_2,
  VS1053_FAILED,
  VS1053_READY
};

const uint8_t VS1053_CHUNK_SIZE = 32;

const uint8_t VS1053_WRITE_OP = 2;
const uint8_t VS1053_READ_OP = 3;

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

class VS1053Component : public Component {
 public:
  void set_dreq_pin(GPIOPin *dreq_pin) { this->dreq_pin_ = dreq_pin; }
  void set_xdcs_pin(GPIOPin *xdcs_pin) { this->xdcs_pin_ = xdcs_pin; }
  void set_xcs_pin(GPIOPin *xcs_pin) { this->xcs_pin_ = xcs_pin; }
  void set_spi(VS1053SPI *spi) { this->spi_ = spi; }

  void setup() override;
  void dump_config() override;
  void loop() override;

 protected:
  State state_{VS1053_INIT};
  uint32_t state_timer_; 
  void to_state_(State state);
  bool state_ms_passed_(uint32_t nr_of_ms) const;

  VS1053SPI *spi_;
  GPIOPin *xcs_pin_;
  GPIOPin *xdcs_pin_;
  GPIOPin *dreq_pin_;

  bool test_communication_();
  void soft_reset_();
  void control_mode_on_();
  void control_mode_off_();
  void data_mode_on_();
  void data_mode_off_();
  void write_register_(uint8_t reg, uint16_t value);
  uint16_t read_register_(uint8_t reg);
  bool data_request_ready_() const;
  void wait_for_data_request_() const;
};

}  // namespace vs1053
}  // namespace esphome
