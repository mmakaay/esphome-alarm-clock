#include "esphome/core/log.h"
#include "blob.h"

namespace esphome {
namespace blob {

static const char *const TAG = "blob";

void Blob::reset() {
  this->pos_ = 0;
}

bool Blob::next_chunk(size_t max_chunk_size, uint8_t** chunk_data, size_t* chunk_size) {
  if (this->pos_ >= this->size ) {
    return false;
  }
  auto len = std::min(max_chunk_size, this->size - this->pos_);
  *chunk_data = (uint8_t*)(this->data + this->pos_);
  *chunk_size = len;
  this->pos_ += len;
  return true;
}

}  // namespace blob
}  // namespace esphome
