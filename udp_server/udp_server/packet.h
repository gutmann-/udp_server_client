#ifndef UDP_SERVER_PACKET_H_
#define UDP_SERVER_PACKET_H_

#include <bits/stdint-uintn.h>
#include <cstddef>
#include <vector>

namespace udp_server {
namespace base {

class BufferReader;
class BufferWriter;

} // namespace base

/**
 * Packet to send and receive from network.
 */
class Packet {
public:
  enum class Type : uint8_t { ACK = 0, PUT = 1, UNKNOWN = 0xff, };
  struct Header {
    uint32_t seq_number;
    uint32_t seq_total;
    Type type;
    uint64_t file_id;
  };

  static const size_t MAX_SIZE;

  static Packet ACK(const Header& to_packet);
  static Packet ACK(const Header& to_packet, uint32_t crc32);

  Packet();
  explicit Packet(base::BufferReader* reader);
  Packet(Packet&& from) noexcept;

  Packet& operator=(Packet&& from) noexcept;

  bool ReadFrom(base::BufferReader* reader);
  void WriteTo(base::BufferWriter* writer) const;

  std::vector<uint8_t> TakeData() { return std::move(data_); }
  void Clear();

  [[nodiscard]] Header header() const { return header_; }
private:
  bool ParseHeader(base::BufferReader* reader);
  bool ParseData(base::BufferReader* reader);

  void WriteHeader(base::BufferWriter* writer) const;
  void WriteData(base::BufferWriter* writer) const;

  Header header_;
  std::vector<uint8_t> data_;
};

} // namespace udp_server

#endif // UDP_SERVER_PACKET_H_
