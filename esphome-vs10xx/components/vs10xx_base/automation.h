#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "vs10xx_base.h"

namespace esphome {
namespace vs10xx_base {

#define VS10XX_SIMPLE_ACTION(ACTION_CLASS, ACTION_METHOD) \
  template<typename... Ts> \
  class ACTION_CLASS : /* NOLINT */ \
                       public Action<Ts...>, \
                       public Parented<VS10XXBase> { \
    void play(Ts... x) override { this->parent_->hal()->ACTION_METHOD(); } \
  };

VS10XX_SIMPLE_ACTION(TurnOffOutputAction, turn_off_output)

template<typename... Ts> class SetVolumeAction : public Action<Ts...>, public Parented<VS10XXBase> {
 public:
  TEMPLATABLE_VALUE(uint8_t, left)
  TEMPLATABLE_VALUE(uint8_t, right)

  void play(Ts... x) override {
    auto left = this->left_.value(x...);
    auto right = this->right_.value(x...);
    auto volume = Volume { left, right };
    this->parent_->hal()->set_volume(volume);
  }
};

}  // namespace vs10xx_base
}  // namespace esphome
