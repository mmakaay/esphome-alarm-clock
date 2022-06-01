#pragma once

#include "esphome/components/vs10xx/vs10xx_plugin.h"

namespace esphome {
namespace vs10xx {

/// The old WAV (RIFF) parser in vs1011e and vs1003b is not very intelligent,
/// it expects a wav file to have the chunks in a specific order. Some files
/// have extra chunks in them that the parser does not know how to skip,
/// so such files do not play at all.
/// 
/// This patch allows the parser to skip unknown chunks in the file.
///
/// File   : wav03b.plg
/// IRAM   : 0x300 .. 0x4b0
/// Compat : incompatible with the "8khzmp3fix" patch
///
/// Hardware or software reset will deactivate the patch. You must reload
/// the patch after each hardware and software reset.
/// 
/// This patch uses the application address to start automatically (the last
/// entry in the patch tables writes to SCI_AIADDR), but does not use it
/// afterwards. So, you must load any patch that actually uses the
/// application address after this patch or it will be deactivated.
///
/// This patch is not compatible with the MPEG2.0 Layer 2 Patch nor with
/// the MPEG2.5 Layer-3 8kHz Stereo Patch.
///
/// See:
/// - https://www.vlsi.fi/en/support/software/vs10xxpatches.html
/// - https://www.vlsi.fi/fileadmin/software/VS10XX/wavfix.pdf
class PluginVS1003WavFix: public VS10XXPlugin {
  using VS10XXPlugin::VS10XXPlugin;

  const char* description() const override {
    return "wavfix: allow WAV parser to skip unknown chunks";
  }

  const std::vector<uint16_t> plugin_data_() override {
    return {
	  0x0007,0x0001, /*copy 1*/
	  0x8030,
	  0x0006,0x0038, /*copy 56*/
	  0x0006,0x2016,0x0000,0x004d,0x0000,0x0d0e,0x2818,0xd5c0,
	  0x0011,0xcc8f,0x0000,0x0d0e,0x001d,0x0800,0x0019,0x9b41,
	  0x6fc2,0x0024,0x0000,0x004d,0x2800,0x1185,0x001d,0x1840,
	  0x0019,0x1841,0x6fc2,0x4513,0x3313,0x0024,0x2811,0xf545,
	  0x001d,0xd840,0x3413,0x184c,0xf400,0x4500,0x0011,0xf44f,
	  0x2811,0xef80,0x0000,0x0d0e,0x0000,0x0406,0x0011,0xee4f,
	  0x2811,0xcdc0,0x0000,0x128e,0x2800,0x0d00,0x4c8e,0x93cc,
	  0x000a,0x0001, /*copy 1*/
	  0x0030,
    };
  }
};

}  // namespace vs10xx
}  // namespace esphome

