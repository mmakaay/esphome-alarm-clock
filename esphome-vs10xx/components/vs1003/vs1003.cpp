#include "vs1003.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vs1003 {

using namespace esphome::vs10xx_base;

static const char *const NAME = "VS1003";
static const char *const TAG = "vs1003";

VS1003Component::VS1003Component() : VS10XXBase(NAME, TAG, CHIPSET_VS1003) {}

}  // namespace vs1003
}  // namespace esphome
