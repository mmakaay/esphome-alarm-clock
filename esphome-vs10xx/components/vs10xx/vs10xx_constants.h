#pragma once

namespace esphome {
namespace vs10xx {

/// The size of the data buffer on the device in bytes. When streaming audio
/// to the device, we must not send more than this in one go.
const uint8_t VS10XX_CHUNK_SIZE = 32;

/// Serial Command Interface (SCI) registers.
const uint8_t SCI_MODE = 0;
const uint8_t SCI_STATUS = 1;
const uint8_t SCI_BASS = 2;
const uint8_t SCI_CLOCKF = 3;
const uint8_t SCI_DECODE_TIME = 4;
const uint8_t SCI_AUDATA = 5;
const uint8_t SCI_WRAM = 6;
const uint8_t SCI_WRAMADDR = 7;
const uint8_t SCI_AIADDR = 10;
const uint8_t SCI_VOL = 11;
const uint8_t SCI_AICTRL0 = 12;
const uint8_t SCI_AICTRL1 = 13;
const uint8_t SCI_NUM_REGISTERS = 15;

/// Serial Command Interface (SCI) mode bits that are used for setting the
/// SCI registers.
const uint16_t SM_DIFF = 1<<0;
const uint16_t SM_LAYER12 = 1<<1;
const uint16_t SM_RESET = 1<<2;
const uint16_t SM_OUTOFWAV = 1<<3;  // up to VS1003
const uint16_t SM_CANCEL = 1<<3;  // from VS1053 on
const uint16_t SM_EARSPEAKER_LO = 1<<4;
const uint16_t SM_TESTS = 1<<5;
const uint16_t SM_STREAM = 1<<6;
const uint16_t SM_EARSPEAKER_HI = 1<<7;
const uint16_t SM_DACT = 1<<8;
const uint16_t SM_SDIORD = 1<<9;
const uint16_t SM_SDISHARE = 1<<10;
const uint16_t SM_SDINEW = 1<<11;
const uint16_t SM_ADPCM = 1<<12;
const uint16_t SM_ADCPM_HP = 1<<13;
const uint16_t SM_LINE_IN = 1<<14;

}  // namespace vs10xx
}  // namespace esphome
