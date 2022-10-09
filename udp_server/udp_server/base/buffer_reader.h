#ifndef UDP_SERVER_BASE_BUFFER_READER_H_
#define UDP_SERVER_BASE_BUFFER_READER_H_

#include <bits/stdint-uintn.h>
#include <cstddef>
#include <vector>

namespace udp_server::base {

/**
 * A simple buffer implementation which appends various data types to
 * buffer in network format (big endian).
 */
class BufferReader {
public:
  BufferReader(const uint8_t* buf, size_t size);

  [[nodiscard]] bool HasBytes(size_t count) const { return pos() + count <= size(); }

  /// Reads various scalar data types.
  /// @return false if there are not enough bytes in the buffer.
  /// @{
  [[nodiscard]] bool Read1(uint8_t* v);
  [[nodiscard]] bool Read4(uint32_t* v);
  [[nodiscard]] bool Read8(uint64_t* v);
  /// @}

  /// Read data from buffer into vector
  /// @param t a place to read a vector
  /// @param count number of elements to read
  /// @return false if there are not enough bytes in the buffer.
  [[nodiscard]] bool ReadToVector(std::vector<uint8_t>* t, size_t count);

  [[nodiscard]] const uint8_t* data() const { return buf_; }
  [[nodiscard]] size_t size() const { return size_; }
  void set_size(size_t size) { size_ = size; }
  [[nodiscard]] size_t pos() const { return pos_; }
private:
  template <typename T>
  [[nodiscard]] bool Read(T* t);
  template <typename T>
  [[nodiscard]] bool ReadNBytes(T* t, size_t num_bytes);

  const uint8_t* buf_;
  size_t size_;
  size_t pos_;
};

} // namespace udp_server::base

#endif // UDP_SERVER_BASE_BUFFER_READER_H_
