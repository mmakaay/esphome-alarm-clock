#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/components/blob/blob.h"
#include "vs10xx.h"

namespace esphome {
namespace vs10xx {

#define VS10XX_SIMPLE_ACTION(ACTION_CLASS, ACTION_METHOD) \
  template<typename... Ts> \
  class ACTION_CLASS : /* NOLINT */ \
                       public Action<Ts...>, \
                       public Parented<VS10XX> { \
    void play(Ts... x) override { this->parent_->hal->ACTION_METHOD(); } \
  };

VS10XX_SIMPLE_ACTION(TurnOffOutputAction, turn_off_output)

template<typename... Ts> class SetVolumeAction : public Action<Ts...>, public Parented<VS10XX> {
 public:
  TEMPLATABLE_VALUE(float, left)
  TEMPLATABLE_VALUE(float, right)

  void play(Ts... x) override {
    auto left = this->left_.value(x...);
    auto right = this->right_.value(x...);
    this->parent_->set_volume(left, right);
  }
};

template<typename... Ts> class ChangeVolumeAction : public Action<Ts...>, public Parented<VS10XX> {
 public:
  TEMPLATABLE_VALUE(float, delta)

  void play(Ts... x) override {
    auto delta = this->delta_.value(x...);
    this->parent_->change_volume(delta);
  }
};

template<typename... Ts> class PlayAction : public Action<Ts...>, public Parented<VS10XX> {
 public:
  TEMPLATABLE_VALUE(blob::Blob*, blob)

  void play(Ts... x) override {
    auto *blob = this->blob_.value(x...);
    this->parent_->play(blob);
  }
};

}  // namespace vs10xx
}  // namespace esphome
