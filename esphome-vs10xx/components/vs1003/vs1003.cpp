#include "vs1003.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vs1003 {

using namespace esphome::vs10xx_base;

static const char *const TAG = "vs1003";

VS1003Component::VS1003Component() : VS10XXBase(TAG, CHIPSET_VS1003) {}

void VS1003Component::setup() {
  VS10XXBase::setup();
}

void VS1003Component::dump_config() {
  ESP_LOGCONFIG(TAG, "VS1003:");
  VS10XXBase::dump_config();
}

void VS1003Component::loop() {
  VS10XXBase::loop();
}

}  // namespace vs1003
}  // namespace esphome
