#include "udp_server/net/address.h"

#include <netinet/in.h>

namespace udp_server::net {

Address::Address() : Address(0) {}

Address::Address(uint16_t port)
        : port_(port) {}

std::vector<uint8_t> Address::SockAddrBytes() const {
  return {
      reinterpret_cast<const uint8_t*>(sockaddr()),
      reinterpret_cast<const uint8_t*>(sockaddr()) + socklen()
  };
}

void Address::Assign(const std::vector<uint8_t>& addr) {
  if (!addr.empty()) {
    Assign(reinterpret_cast<const struct sockaddr*>(addr.data()), addr.size());
  } else {
    Reset();
  }
}

void Address::Assign(const struct sockaddr* addr, socklen_t addrlen) {
  if (addr) {
    const auto address = reinterpret_cast<const sockaddr_in*>(addr);
    port_ = ntohs(address->sin_port);
  } else {
    Reset();
  }
}

void Address::Reset() {
  port_ = 0;
}

uint16_t Address::port() const { return port_; }

} // namespace udp_server::net