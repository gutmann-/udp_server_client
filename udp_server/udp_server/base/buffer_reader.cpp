#include "udp_server/base/buffer_reader.h"

#include <bits/stdint-intn.h>

namespace udp_server::base {

BufferReader::BufferReader(const uint8_t* buf, size_t size)
             : buf_(buf),
               size_(size),
               pos_(0) {}

bool BufferReader::Read1(uint8_t* v) {
  if (!HasBytes(1))
    return false;
  *v = buf_[pos_++];
  return true;
}

bool BufferReader::Read4(uint32_t* v) {
  return Read(v);
}

bool BufferReader::Read8(uint64_t* v) {
  return Read(v);
}

bool BufferReader::ReadToVector(std::vector<uint8_t>* vec, size_t count) {
  if (!HasBytes(count))
    return false;
  vec->assign(buf_ + pos_, buf_ + pos_ + count);
  pos_ += count;
  return true;
}

template <typename T>
bool BufferReader::Read(T* v) {
  return ReadNBytes(v, sizeof(*v));
}

template <typename T>
bool BufferReader::ReadNBytes(T* v, size_t num_bytes) {
  if (!HasBytes(num_bytes))
    return false;

  // Sign extension is required only if
  //     |num_bytes| is less than size of T, and
  //     T is a signed type.
  const bool sign_extension_required =
      num_bytes < sizeof(*v) && static_cast<T>(-1) < 0;
  // Perform sign extension by casting the byte value to int8_t, which will be
  // sign extended automatically when it is implicitly converted to T.
  T tmp = sign_extension_required ? static_cast<int8_t>(buf_[pos_++])
                                  : buf_[pos_++];
  for (size_t i = 1; i < num_bytes; ++i) {
    tmp <<= 8;
    tmp |= buf_[pos_++];
  }
  *v = tmp;
  return true;
}

} // namespace udp_server::base