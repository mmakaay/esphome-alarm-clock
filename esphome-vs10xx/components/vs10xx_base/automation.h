#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/components/blob/blob.h"
#include "vs10xx_base.h"

namespace esphome {
namespace vs10xx_base {

#define VS10XX_SIMPLE_ACTION(ACTION_CLASS, ACTION_METHOD) \
  template<typename... Ts> \
  class ACTION_CLASS : /* NOLINT */ \
                       public Action<Ts...>, \
                       public Parented<VS10XXBase> { \
    void play(Ts... x) override { this->parent_->hal->ACTION_METHOD(); } \
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
    this->parent_->hal->set_volume(volume);
  }
};

template<typename... Ts> class PlayAction : public Action<Ts...>, public Parented<VS10XXBase> {
 public:
  TEMPLATABLE_VALUE(blob::Blob*, blob)

  void play(Ts... x) override {
    auto *blob = this->blob_.value(x...);
    this->parent_->play(blob);
  }
};

}  // namespace vs10xx_base
}  // namespace esphome
