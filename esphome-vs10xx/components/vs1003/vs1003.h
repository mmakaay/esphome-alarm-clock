#pragma once

#include "esphome/components/vs10xx_base/vs10xx_base.h"

namespace esphome {
namespace vs1003 {

class VS1003Component : public vs10xx_base::VS10XXBase {
 public:
  explicit VS1003Component();
  void setup() override;
  void dump_config() override;
  void loop() override;
};

}  // namespace vs1003
}  // namespace esphome
