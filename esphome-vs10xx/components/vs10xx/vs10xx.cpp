#include "vs10xx.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vs10xx {

static const char *const TAG = "vs10xx";

const char* device_state_to_text(DeviceState state) {
  switch (state) {
    case DEVICE_RESET:
      return "Performing hard reset";
    case DEVICE_VERIFY_CHIPSET:
      return "Checking chipset";
    case DEVICE_SOFT_RESET:
      return "Soft resetting";
    case DEVICE_TO_FAST_SPI:
      return "Setting SPI to fast mode";
    case DEVICE_LOAD_PLUGINS:
      return "Loading plugins";
    case DEVICE_INIT_AUDIO:
      return "Initializing audio";
    case DEVICE_REPORT_FAILED:
      return "Reporting a device failure";
    case DEVICE_FAILED:
      return "Device has failed";
    case DEVICE_READY:
      return "Ready for use";
    default:
      return "Unknown state";
  }
}

const char* media_state_to_text(MediaState state) {
  switch (state) {
    case MEDIA_STOPPED:
      return "Not playing";
    case MEDIA_STARTING:
      return "Starting playback";
    case MEDIA_PLAYING:
      return "Playing audio file";
    case MEDIA_STOPPING:
      return "Stopping playback";
    default:
      return "Unknown state";
  }
}

void VS10XX::dump_config() {
  ESP_LOGCONFIG(TAG, "VS10XX:");
  this->hal->log_config();
  if (this->plugins_.size() > 0) {
    ESP_LOGCONFIG(TAG, "  Plugins:");
    for (auto *plugin : this->plugins_) {
      ESP_LOGCONFIG(TAG, "    - %s", plugin->description());
    }
  }
}

void VS10XX::setup() {
  ESP_LOGCONFIG(TAG, "Setting up device");
  this->preferences_store_ = global_preferences->make_preference<VS10XXPreferences>(this->get_object_id_hash());
}

void VS10XX::loop() {
  switch (this->device_state_) {
    case DEVICE_READY:
      this->handle_media_operations_();
      break;
    case DEVICE_RESET:
      if (this->hal->reset()) {
        this->set_device_state_(DEVICE_VERIFY_CHIPSET);
      } else {
        this->set_device_state_(DEVICE_REPORT_FAILED);
      }
      break;
    case DEVICE_VERIFY_CHIPSET:
      if (this->hal->go_slow() &&
          this->hal->test_communication() &&
          this->hal->verify_chipset()) {
        this->set_device_state_(DEVICE_SOFT_RESET);
      } else {
        this->set_device_state_(DEVICE_REPORT_FAILED);
      }
      break;
    case DEVICE_SOFT_RESET:
      if (this->hal->go_slow() && this->hal->soft_reset()) {
        this->set_device_state_(DEVICE_TO_FAST_SPI);
      } else {
        this->set_device_state_(DEVICE_REPORT_FAILED);
      }
    case DEVICE_TO_FAST_SPI:
      if (this->hal->go_fast() && this->hal->test_communication()) {
        this->set_device_state_(DEVICE_LOAD_PLUGINS);
      } else {
        this->set_device_state_(DEVICE_REPORT_FAILED);
      }
    case DEVICE_LOAD_PLUGINS:
      for (auto *plugin : this->plugins_) {
        ESP_LOGD(TAG, "Loading plugin: %s", plugin->description());
        if (!plugin->load(this->hal)) {
          this->set_device_state_(DEVICE_REPORT_FAILED);
          return;
        }
      }
      this->set_device_state_(DEVICE_INIT_AUDIO);
    case DEVICE_INIT_AUDIO:
      this->hal->turn_on_output();
      this->restore_preferences_();
      this->sync_preferences_to_device_();
      this->set_device_state_(DEVICE_READY); 
      ESP_LOGI(TAG, "Device initialized successfully");
      break;
    case DEVICE_REPORT_FAILED:
      ESP_LOGE(TAG, "Device failed");
      this->set_device_state_(DEVICE_FAILED);
      break;
    case DEVICE_FAILED:
      // NOOP
      break;
  }
}

void VS10XX::handle_media_operations_() {
  // First, try to send async preference changes to the device.
  // If this does not work, we'll try again the next time.
  this->sync_preferences_to_device_();

  // Secondly, handle playing media.
  auto start = millis();
  size_t sent = 0;
  switch (this->media_state_) {
    case MEDIA_STOPPED:
      if (this->next_audio_ != nullptr) {
        this->audio_ = this->next_audio_;
        this->next_audio_ = nullptr;
        this->media_state_ = MEDIA_STARTING;
      }
      break;
    case MEDIA_STARTING:
      this->audio_->reset();
      this->high_freq_.start();
      this->hal->reset_decode_time();
      this->media_state_ = MEDIA_PLAYING;
      break;
    case MEDIA_PLAYING:
      // Send chunks of data to the device. Limit the time during which
      // this is done, to not block the main loop for too long.
      // Also, when a change in the settings is detected, then stop feeding
      // audio, to allow the change to be propagated to the device.
      while ((millis() - start) < 30 && this->changed_preferences_ == CHANGE_NONE) {
        if (this->hal->is_ready()) {
          if (this->audio_->next_chunk(VS10XX_CHUNK_SIZE)) {
            if (this->audio_->chunk_size > 0) {
              this->hal->begin_data_transaction();
              for (size_t i = 0; i < this->audio_->chunk_size; i++) {
                  this->hal->write_byte(*(this->audio_->chunk_start + i));
                  sent++;
              }
              this->hal->end_transaction();
            }
          } else {
            // Out of audio
            ESP_LOGD(TAG, "Reached end of media input");
            this->media_state_ = MEDIA_STOPPING;
            return;
          }
        }
      }
      break;
    case MEDIA_STOPPING:
      this->high_freq_.stop();
      this->device_state_ = DEVICE_SOFT_RESET;
      this->media_state_ = MEDIA_STOPPED;
      break;
  }
}

void VS10XX::set_device_state_(DeviceState state) {
  this->device_state_ = state;
  ESP_LOGD(TAG, "Device state: [%d] %s", state, device_state_to_text(state));
}

void VS10XX::set_media_state_(MediaState state) {
  this->media_state_ = state;
  ESP_LOGD(TAG, "Media state: [%d] %s", state, media_state_to_text(state));
}

void VS10XX::set_volume(float left, float right, bool publish) {
  auto left_ = clamp(left, 0.0f, 1.0f);
  auto right_ = clamp(right, 0.0f, 1.0f);
  ESP_LOGD(TAG, "Set output volume: left=%0.2f, right=%0.2f", left_, right_);

  this->preferences_.volume_left = left_;
  this->preferences_.volume_right = right_;
  this->changed_preferences_ |= CHANGE_VOLUME;

  if (publish) { this->store_preferences_(); }
}

void VS10XX::change_volume(float delta) {
  auto delta_ = clamp(delta, -1.0f, 1.0f);
  ESP_LOGD(TAG, "Change output volume: delta=%0.2f", delta_);
  this->set_volume(this->preferences_.volume_left + delta_,
                   this->preferences_.volume_right + delta_, true);
}

void VS10XX::play(blob::Blob *blob) {
  if (this->device_state_ != DEVICE_READY) {
    ESP_LOGE(TAG, "play(): Device not ready (current state: %s)", device_state_to_text(this->device_state_));
  } else if (this->media_state_ == MEDIA_STOPPED) {
    ESP_LOGD(TAG, "play(): starting playback");
    this->set_media_state_(MEDIA_STARTING);
    this->audio_ = blob;
  } else if (this->media_state_ == MEDIA_PLAYING) {
    ESP_LOGD(TAG, "play(): Already playing, first stopping active playback");
    this->next_audio_ = blob;
    this->set_media_state_(MEDIA_STOPPING);
  } else {
    ESP_LOGE(TAG, "play(): Current media state (%s) not supported play command", media_state_to_text(this->media_state_));
  }
}

void VS10XX::stop() {
  if (this->device_state_ != DEVICE_READY) {
    ESP_LOGE(TAG, "stop(): Device not ready (current state: %s)", device_state_to_text(this->device_state_));
  } else if (this->media_state_ == MEDIA_STOPPED) {
    ESP_LOGD(TAG, "stop(): Media already stopped, OK");
  } else if (this->media_state_ == MEDIA_STOPPING) {
    ESP_LOGD(TAG, "stop(): Media already stopping, OK");
  } else {
    ESP_LOGD(TAG, "stop(): Stopping media playback");
    this->media_state_ = MEDIA_STOPPING;
  }
}

void VS10XX::store_preferences_() {
  this->preferences_store_.save(&this->preferences_); 
  ESP_LOGD(TAG, "Preferences stored");
}

void VS10XX::restore_preferences_() {
  if (!this->preferences_store_.load(&this->preferences_)) {
    ESP_LOGW(TAG, "Restoring preferences failed, using defaults");
    this->set_default_preferences_();
  } else {
    ESP_LOGD(TAG, "Preferences restored");
  }
  this->changed_preferences_ = CHANGE_ALL;

  ESP_LOGD(TAG, "  - Volume left  : %0.2f", this->preferences_.volume_left);
  ESP_LOGD(TAG, "  - Volume right : %0.2f", this->preferences_.volume_right);
  ESP_LOGD(TAG, "  - Muted        : %s", YESNO(this->preferences_.muted));
}

void VS10XX::sync_preferences_to_device_() {
  if (this->changed_preferences_ == CHANGE_NONE) {
    return;
  }
  if (this->changed_preferences_ & CHANGE_VOLUME) {
    auto left = this->preferences_.volume_left;
    auto right = this->preferences_.volume_right;
    if (this->hal->set_volume(left, right)) {
      this->changed_preferences_ &= ~CHANGE_VOLUME;
    }
  }
}

void VS10XX::set_default_preferences_() {
  this->preferences_.volume_left = 1.0f;
  this->preferences_.volume_right = 1.0f;
  this->preferences_.muted = false;
}

//uint32_t VS10XX::hash_base() { return 1527366430UL; }

}  // namespace vs10xx
}  // namespace esphome
