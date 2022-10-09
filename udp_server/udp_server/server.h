#ifndef UDP_SERVER_SERVER_H_
#define UDP_SERVER_SERVER_H_

#include "udp_server/file.h"
#include "udp_server/packet.h"
#include "udp_server/net/udp_socket.h"

#include <functional>
#include <unordered_map>

namespace udp_server {

class Server {
public:
  explicit Server(net::UDPSocket&& socket);

  void Run();

  void OnNewFile(const std::function<void(const File& file, uint32_t crc32)>& handler);
private:
  std::pair<ssize_t, std::unique_ptr<net::Address>> ReceivePacket(Packet* receive_into);
  void AddPacket(Packet&& packet);

  void SendACK(const net::Address& to, const Packet::Header& header);
  Packet MakeACKPacket(const Packet::Header& header);
  uint32_t CalculateCrc32(const File& file);

  net::UDPSocket socket_;
  std::unordered_map<uint64_t, File> files_;
  std::unordered_map<uint64_t, uint32_t> crc32_;

  std::function<void(const File& file, uint32_t crc32)> on_new_file_;
};

} // namespace udp_server

#endif // UDP_SERVER_SERVER_H_
