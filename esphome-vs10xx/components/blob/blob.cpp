#include "esphome/core/log.h"
#include "blob.h"

namespace esphome {
namespace blob {

static const char *const TAG = "blob";

void Blob::reset() {
  this->pos_ = 0;
  this->chunk_size = 0;
  this->chunk_start = this->data;
}

bool Blob::next_chunk(size_t max_chunk_size) {
  if (this->pos_ >= this->size ) {
    return false;
  }
  auto len = std::min(max_chunk_size, this->size - this->pos_);
  this->chunk_start = (uint8_t*)(this->data + this->pos_);
  this->chunk_size = len;
  this->pos_ += len;
  return true;
}

}  // namespace blob
}  // namespace esphome

