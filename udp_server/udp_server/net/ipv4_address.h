#ifndef UDP_SERVER_NET_IPV4_ADDRESS_H_
#define UDP_SERVER_NET_IPV4_ADDRESS_H_

#include "udp_server/net/address.h"

#include <array>
#include <netinet/in.h>

namespace udp_server::net {

/**
 * IPv4 address.
 */
class IPv4Address : public Address {
public:
  IPv4Address();
  explicit IPv4Address(uint16_t port);
  IPv4Address(const IPv4Address&) = default;
  IPv4Address(const std::array<uint8_t, 4>& address, int port);

  IPv4Address& operator =(const IPv4Address&) = default;

  [[nodiscard]] in_addr_t inet_addr() const;

  void Assign(const struct sockaddr* addr, socklen_t addrlen) override;
  void Reset() override;
  [[nodiscard]] std::unique_ptr<Address> Clone() const override;

  [[nodiscard]] const struct sockaddr* sockaddr() const override;
  [[nodiscard]] socklen_t socklen() const override;
private:
  std::array<uint8_t, 4> address_; // octetes of the ipv4 address
  sockaddr_in sockaddr_in_;
};

} // namespace udp_server::net

#endif // UDP_SERVER_NET_IPV4_ADDRESS_H_
