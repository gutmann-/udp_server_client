#include "udp_server/packet.h"

#include "udp_server/base/buffer_reader.h"
#include "udp_server/base/buffer_writer.h"

namespace udp_server {

// static
const size_t Packet::MAX_SIZE = 1472;

// static
Packet Packet::ACK(const Packet::Header& to_packet) {
  auto ack = Packet();
  ack.header_ = to_packet;
  ack.header_.type = Type::ACK;
  return ack;
}

// static
Packet Packet::ACK(const Header& to_packet, uint32_t crc32) {
  base::BufferWriter crc_writer;
  crc_writer.AppendInt(crc32);

  Packet ack = ACK(to_packet);
  ack.data_ = crc_writer.TakeBuf();
  return ack;
}

Packet::Packet()
       : header_(Header {
         .seq_number = 0,
         .seq_total = 0,
         .type = Type::UNKNOWN,
         .file_id = 0,
         }),
         data_() {}

Packet::Packet(base::BufferReader* reader)
       : Packet() { ReadFrom(reader); }

Packet::Packet(Packet&& from) noexcept
       : header_(from.header_),
         data_(std::move(from.data_)) {}

Packet& Packet::operator=(Packet&& from) noexcept {
  if (&from != this) {
    header_ = from.header_;
    data_ = std::move(from.data_);
  }

  return *this;
}

bool Packet::ReadFrom(base::BufferReader* reader) {
  if (!reader) return false;
  return ParseHeader(reader) && ParseData(reader);
}

void Packet::WriteTo(base::BufferWriter* writer) const {
  if (!writer) return;

  WriteHeader(writer);
  WriteData(writer);
}

void Packet::Clear() {
  *this = Packet();
}

bool Packet::ParseHeader(base::BufferReader* reader) {
  uint8_t type = 0;

  const auto parsed =
      reader->Read4(&header_.seq_number) &&
      reader->Read4(&header_.seq_total)  &&
      reader->Read1(&type)               &&
      reader->Read8(&header_.file_id);
  header_.type = static_cast<Type>(type);

  return parsed;
}

bool Packet::ParseData(base::BufferReader* reader) {
  const size_t data_size = reader->size() - reader->pos();
  return reader->ReadToVector(&data_, data_size);
}

void Packet::WriteHeader(base::BufferWriter* writer) const {
  writer->AppendInt(header_.seq_number);
  writer->AppendInt(header_.seq_total);
  writer->AppendInt(static_cast<uint8_t>(header_.type));
  writer->AppendInt(header_.file_id);
}

void Packet::WriteData(base::BufferWriter* writer) const {
  if (!data_.empty())
    writer->AppendVector(data_);
}

} // namespace udp_server