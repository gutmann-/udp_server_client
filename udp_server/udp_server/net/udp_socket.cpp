#include "udp_server/net/udp_socket.h"

namespace udp_server::net {

UDPSocket::UDPSocket()
          : Socket(AF_INET, SOCK_DGRAM, 0) {}

ssize_t UDPSocket::Recv(uint8_t* buf, size_t len, int flags) {
  if (bound_to()) {
    return recv(socket_fd(), buf, len, flags);
  } else {
    return -1;
  }
}

ssize_t UDPSocket::Recv(std::vector<uint8_t>* to, int flags) {
  return Recv(to->data(), to->size(), flags);
}

std::pair<ssize_t, std::unique_ptr<Address>>
      UDPSocket::RecvFrom(uint8_t* buf, size_t len, int flags) {
  std::pair<ssize_t, std::unique_ptr<Address>> result = { -1, nullptr };
  if (bound_to()) {
    auto recvfrom_addr = bound_to()->SockAddrBytes();
    auto recvfrom_len = bound_to()->socklen();
    const auto bytes_received = recvfrom(socket_fd(), buf, len, flags,
                                         reinterpret_cast<struct sockaddr*>(recvfrom_addr.data()),
                                         &recvfrom_len);

    if (bytes_received > 0) {
      auto received_from_address = bound_to()->Clone();
      received_from_address->Assign(recvfrom_addr);

      result = std::make_pair(bytes_received, std::move(received_from_address));
    } else {
      result.first = bytes_received;
    }
  }

  return result;
}

std::pair<ssize_t, std::unique_ptr<Address>>
    UDPSocket::RecvFrom(std::vector<uint8_t>* to, int flags) {
  return RecvFrom(to->data(), to->size(), flags);
}

ssize_t UDPSocket::SendTo(const Address& to, const uint8_t* buf, size_t len, int flags) {
  return sendto(socket_fd(), buf, len, flags, to.sockaddr(), to.socklen());
}

ssize_t UDPSocket::SendTo(const udp_server::net::Address& to, const std::vector<uint8_t>& what, int flags) {
  return SendTo(to, what.data(), what.size(), flags);
}

} // namespace udp_server::net