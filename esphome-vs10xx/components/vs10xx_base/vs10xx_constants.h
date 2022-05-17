#pragma once

namespace esphome {
namespace vs10xx_base {

/// The size of the data buffer on the device in bytes. When streaming audio
/// to the device, we must not send more than this in one go.
const uint8_t VS10XX_CHUNK_SIZE = 32;

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

}  // namespace vs10xx
}  // namespace esphome
