#pragma once

#include "esphome/components/vs10xx/vs10xx_plugin.h"

namespace esphome {
namespace vs10xx {

/// When you need to play stereo files but only use one of the
/// analog outputs, this is the patch for you. The VS1003 Mono
/// Patch replaces the regular DAC interrupt handler and plays
/// all output as mono.
///
/// File : dacpatch.plg
/// IRAM : 0x4e0 .. 0x4f3
///
/// When you load the patch, it is automatically installed into
/// the DAC interrupt handler. If you want to disable the patch,
/// give a software reset.
///
/// See:
/// - https://www.vlsi.fi/en/support/software/vs10xxpatches.html
/// - https://www.vlsi.fi/fileadmin/software/VS10XX/dacpatch.pdf
class PluginVS1003DacMono: public VS10XXPlugin {
  using VS10XXPlugin::VS10XXPlugin;

  const char* description() const override {
    return "dacmono: play all DAC output as mono";
  }

  const std::vector<uint16_t> plugin_data_() override {
    return {
      0x0007,0x0001, /*copy 1*/
      0x84e0,
      0x0006,0x0024, /*copy 36*/
      0x3e02,0xb851,0x3e14,0xf812,0x3e11,0xb817,0x0006,0x5597,
      0x0023,0xffd2,0x3e01,0x1c13,0x3009,0x0e06,0xf168,0x8e06,
      0x0030,0x0551,0xf16c,0x0024,0x464c,0x1bc4,0x3911,0x8024,
      0x3961,0xbc13,0x36f1,0x9817,0x36f4,0xd812,0x3602,0x8024,
      0x2100,0x0000,0x3904,0x5bd1,
      0x0007,0x0001, /*copy 1*/
      0x8020,
      0x0006,0x0002, /*copy 2*/
      0x2a01,0x380e,
    };
  }
};

}  // namespace vs10xx
}
// namespace esphome

