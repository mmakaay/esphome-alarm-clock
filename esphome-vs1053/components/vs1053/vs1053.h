#pragma once

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
  SM_DIFF = 0,
  SM_LAYER12 = 1,
  SM_RESET = 2,
  SM_OUTOFWAV = 3,
  SM_EARSPEAKER_LO = 4,
  SM_TESTS = 5,
  SM_STREAM = 6,
  SM_EARSPEAKER_HI = 7,
  SM_DACT = 8,
  SM_SDIORD = 9,
  SM_SDISHARE = 10,
  SM_SDINEW = 11,
  SM_ADPCM = 12,
  SM_ADCPM_HP = 13,
  SM_LINE_IN = 14,
};

class VS1053SlowSPI : public spi::SPIDevice<
                            spi::BIT_ORDER_MSB_FIRST,
                            spi::CLOCK_POLARITY_LOW,
                            spi::CLOCK_PHASE_LEADING,
                            spi::DATA_RATE_200KHZ> { };

class VS1053FastSPI : public spi::SPIDevice<
                            spi::BIT_ORDER_MSB_FIRST,
                            spi::CLOCK_POLARITY_LOW,
                            spi::CLOCK_PHASE_LEADING,
                            spi::DATA_RATE_4MHZ> { };

class VS1053Component : public Component, public VS1053SlowSPI {
 public:
  void set_dreq_pin(GPIOPin *dreq_pin) { this->dreq_pin_ = dreq_pin; }
  void set_xdcs_pin(GPIOPin *xdcs_pin) { this->xdcs_pin_ = xdcs_pin; }
  void set_xcs_pin(GPIOPin *xcs_pin) { this->xcs_pin_ = xcs_pin; }
  //void set_slow_spi(VS1053SlowSPI *spi) { this->slow_spi_ = spi; }
  //void set_fast_spi(VS1053FastSPI *spi) { this->fast_spi_ = spi; }

  void setup() override;
  void dump_config() override;
  void loop() override;

 protected:
  State state_{VS1053_INIT};
  uint32_t state_timer_; 
  //VS1053SlowSPI *slow_spi_;
  //VS1053FastSPI *fast_spi_;
  bool fast_mode_{false};
  GPIOPin *xcs_pin_;
  GPIOPin *xdcs_pin_;
  GPIOPin *dreq_pin_;
  void to_state_(State state);
  bool state_ms_passed_(uint32_t nr_of_ms) const;
  bool test_communication_();
  void soft_reset_();
  void control_mode_on_();
  void control_mode_off_();
  void data_mode_on_();
  void data_mode_off_();
  void write_register_(uint8_t reg, uint16_t value);
  uint16_t read_register_(uint8_t reg);
  void wait_for_data_request_() const;
};

}  // namespace vs1053
}  // namespace esphome
