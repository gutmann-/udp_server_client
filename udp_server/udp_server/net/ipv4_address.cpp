#include "udp_server/net/ipv4_address.h"

#include <netinet/in.h>

namespace udp_server::net {
namespace {

std::array<uint8_t, 4> AddressFrom(in_addr_t addr) {
  return {
    static_cast<uint8_t>(addr       & 0xff),
    static_cast<uint8_t>(addr >> 8  & 0xff),
    static_cast<uint8_t>(addr >> 16 & 0xff),
    static_cast<uint8_t>(addr >> 24 & 0xff),
  };
}

} // namespace

IPv4Address::IPv4Address() : IPv4Address(0) {}

IPv4Address::IPv4Address(uint16_t port)
            : IPv4Address(std::array<uint8_t, 4>{ 0, 0, 0, 0 }, port) {}

IPv4Address::IPv4Address(const std::array<uint8_t, 4>& address, int port)
            : Address(port),
              address_(address),
              sockaddr_in_ {
                  .sin_family = AF_INET,
                  .sin_port = htons(this->port()),
                  .sin_addr = { .s_addr = inet_addr() },
              } {}

in_addr_t IPv4Address::inet_addr() const {
  return
    address_[0]       |
    address_[1] << 8  |
    address_[2] << 16 |
    address_[3] << 24 ;
}

void IPv4Address::Assign(const struct sockaddr* addr, socklen_t addrlen) {
  Address::Assign(addr, addrlen);

  if (addr) {
    sockaddr_in_ = *reinterpret_cast<const sockaddr_in*>(addr);
    address_ = AddressFrom(sockaddr_in_.sin_addr.s_addr);
  } else {
    Reset();
  }
}

void IPv4Address::Reset() {
  *this = IPv4Address();
}

std::unique_ptr<Address> IPv4Address::Clone() const {
  return std::make_unique<IPv4Address>(*this);
}

const struct sockaddr* IPv4Address::sockaddr() const {
  return reinterpret_cast<const struct sockaddr*>(&sockaddr_in_);
}

socklen_t IPv4Address::socklen() const { return sizeof(sockaddr_in); }

} // namespace udp_server::net