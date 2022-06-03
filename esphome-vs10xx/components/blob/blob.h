#pragma once

#include "esphome/core/optional.h"

namespace esphome {
namespace blob {

/// The Blob class holds random binary data + its length.
/// Blob data are added to the firmware at compile time and one instance
/// of the Blob is created. Other code can make use of a BlobAccess object
/// to access the data in the Blob.
class Blob {
 public:
  explicit Blob(const uint8_t* data, size_t size) : data(data), size(size) {}

  /// A pointer to the data that are stored in the Blob object.
  const uint8_t* data;

  /// The size of the data (in bytes) that are stored in the Blob object.
  const size_t size;

  /// A pointer to the start of the data of the current chunk.
  const uint8_t* chunk_start{nullptr};

  /// The size of the current chunk.
  size_t chunk_size{0};

  void reset();

  bool next_chunk(size_t max_chunk_size);

 protected:
  size_t pos_{0}; 
};

}  // namespace blob
}  // namespace esphome
