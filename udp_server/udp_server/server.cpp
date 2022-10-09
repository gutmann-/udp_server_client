#include "udp_server/server.h"

#include "udp_server/base/buffer_reader.h"
#include "udp_server/base/buffer_writer.h"
#include "udp_server/base/crc32.h"
#include "udp_server/packet.h"

namespace udp_server {
namespace {

const auto SOCK_SEND_FLAGS = MSG_WAITALL;
const auto SOCK_RECV_FLAGS = MSG_WAITALL;

std::vector<uint8_t> Serialize(const Packet& packet) {
  base::BufferWriter writer;
  packet.WriteTo(&writer);

  return writer.TakeBuf();
}

} // namespace

Server::Server(net::UDPSocket&& socket)
       : socket_(std::move(socket)),
         files_(),
         on_new_file_() {}

void Server::Run() {
  while (true) {
    Packet packet;
    auto [bytes_received, address] = ReceivePacket(&packet);
    if (bytes_received < 0) break;

    const Packet::Header header = packet.header();
    AddPacket(std::move(packet));
    SendACK(*address, header);
  }
}

void Server::OnNewFile(const std::function<void(const File& file, uint32_t crc32)>& handler) {
  on_new_file_ = handler;
}

std::pair<ssize_t, std::unique_ptr<net::Address>>
    Server::ReceivePacket(Packet* receive_into) {
  std::vector<uint8_t> datagram;
  datagram.resize(Packet::MAX_SIZE);

  auto [bytes_received, addr] = socket_.RecvFrom(&datagram, SOCK_RECV_FLAGS);

  if (bytes_received > 0) {
    base::BufferReader buffer_reader(datagram.data(), bytes_received);
    receive_into->ReadFrom(&buffer_reader);
  }

  return std::make_pair(bytes_received, std::move(addr));
}

void Server::AddPacket(Packet&& packet) {
  const auto file_id = packet.header().file_id;
  auto file_it = files_.find(file_id);
  if (file_it == files_.end()) {
    file_it = files_.emplace(std::piecewise_construct,
                             std::forward_as_tuple(file_id),
                             std::forward_as_tuple(file_id, packet.header().seq_total)).first;
  }

  auto& file = file_it->second;
  file.AddSegment(std::move(packet));

  if (file.full() && on_new_file_ && !crc32_.contains(file_id)) {
    const auto crc32 = CalculateCrc32(file);
    on_new_file_(file, crc32);
  }
}

void Server::SendACK(const net::Address& to, const Packet::Header& header) {
  const auto ack_packet = MakeACKPacket(header);
  const auto datagram = Serialize(ack_packet);

  socket_.SendTo(to, datagram, SOCK_SEND_FLAGS);
}

Packet Server::MakeACKPacket(const Packet::Header& header) {
  const auto& file = files_.at(header.file_id);

  auto ack_header = header;
  ack_header.seq_total = file.size();

  if (file.full()) {
    const auto crc32 = CalculateCrc32(file);
    return Packet::ACK(ack_header, crc32);
  } else {
    return Packet::ACK(ack_header);
  }
}

uint32_t Server::CalculateCrc32(const File& file) {
  auto crc_it = crc32_.find(file.id());
  if (crc_it == crc32_.end()) {
    const auto crc32 = base::Crc32(file.begin(), file.end());
    crc_it = crc32_.emplace(file.id(), crc32).first;
  }

  return crc_it->second;
}

} // namespace udp_server