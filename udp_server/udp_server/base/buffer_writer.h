#ifndef UDP_SERVER_BASE_BUFFER_WRITER_H_
#define UDP_SERVER_BASE_BUFFER_WRITER_H_

#include <bits/stdint-uintn.h>
#include <cstddef>
#include <vector>

namespace udp_server::base {

/**
 * A simple buffer writer implementation which appends various data types to
 * buffer.
 */
class BufferWriter {
public:
  /// Append integers to buffer
  /// @{
  void AppendInt(uint8_t v);
  void AppendInt(uint32_t v);
  void AppendInt(uint64_t v);
  /// @}

  void AppendVector(const std::vector<uint8_t>& v);
  void AppendArray(const uint8_t* buf, size_t size);

  /// Takes a buffer from writer. After taking a buffer the writer will contain
  /// empty internal buffer.
  /// @return buffer
  std::vector<uint8_t> TakeBuf() { return std::move(buf_); }
private:
  template <typename T>
  void AppendInternal(T v);

  std::vector<uint8_t> buf_;
};

} // namespace udp_server::base

#endif // UDP_SERVER_BASE_BUFFER_WRITER_H_
