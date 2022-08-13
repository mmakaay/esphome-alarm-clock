#pragma once

// This include contains definitions as provided by the VS10XX manufacturer.
#include "vs10xx_uc.h"

namespace esphome {
namespace vs10xx {

/// The size of the data buffer on the device in bytes. When streaming audio
/// to the device, we must not send more than this in one go.
const uint8_t VS10XX_CHUNK_SIZE = 32;

enum AudioFormat {
  FORMAT_UNKNOWN,
  FORMAT_WAV,
  FORMAT_AAC_ADTS,
  FORMAT_AAC_ADIF,
  FORMAT_AAC_MP4,
  FORMAT_MP3,
  FORMAT_WMA,
  FORMAT_MIDI,
  FORMAT_OGG,
};

}  // namespace vs10xx
}  // namespace esphome
