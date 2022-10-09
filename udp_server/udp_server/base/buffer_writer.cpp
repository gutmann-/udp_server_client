#include "udp_server/base/buffer_writer.h"

#include "udp_server/base/sys_byteorder.h"

namespace udp_server::base {

void BufferWriter::AppendInt(uint8_t v) {
  buf_.push_back(v);
}

void BufferWriter::AppendInt(uint32_t v) {
  AppendInternal(base::HostToNet32(v));
}

void BufferWriter::AppendInt(uint64_t v) {
  AppendInternal(base::HostToNet64(v));
}

void BufferWriter::AppendVector(const std::vector<uint8_t>& v) {
  buf_.insert(buf_.end(), v.begin(), v.end());
}

void BufferWriter::AppendArray(const uint8_t* buf, size_t size) {
  buf_.insert(buf_.end(), buf, buf + size);
}

template <typename T>
void BufferWriter::AppendInternal(T v) {
  AppendArray(reinterpret_cast<uint8_t*>(&v), sizeof(T));
}

} // namespace udp_server