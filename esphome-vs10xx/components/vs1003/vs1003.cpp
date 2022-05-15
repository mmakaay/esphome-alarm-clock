#include "vs1003.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vs1003 {

static const char *const TAG = "vs1003";

VS1003Component::VS1003Component() : vs10xx_base::VS10XXBase(TAG, vs10xx_base::SS_VER_VS1003) {}

void VS1003Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up VS1003Component");
  vs10xx_base::VS10XXBase::setup();
}

void VS1003Component::dump_config() {
  ESP_LOGCONFIG(TAG, "VS1003:");
  vs10xx_base::VS10XXBase::dump_config();
}

void VS1003Component::loop() {
  vs10xx_base::VS10XXBase::loop();
}

}  // namespace vs1003
}  // namespace esphome
