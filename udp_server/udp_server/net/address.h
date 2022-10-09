#ifndef UDP_SERVER_NET_ADDRESS_H_
#define UDP_SERVER_NET_ADDRESS_H_

#include <bits/stdint-uintn.h>
#include <memory>
#include <sys/socket.h>
#include <vector>

struct sockaddr;

namespace udp_server::net {

/**
 * A network address representations.
 * Because this is an abstract class you can not
 * construct address from address string or something similar,
 * you can specify port number only.
 */
class Address {
public:
  Address();
  explicit Address(uint16_t port);
  Address(const Address&) = default;
  virtual ~Address() = default;

  Address& operator =(const Address&) = default;

  /**
   * Constructs sockaddr struct which you can use in
   * function like `bind`
   * @return byte representation of struct sockaddr
   */
  [[nodiscard]] std::vector<uint8_t> SockAddrBytes() const;

  /**
   * Assign new address.
   * @param addr byte representation of struct sockaddr
   */
  void Assign(const std::vector<uint8_t>& addr);
  /**
   * Assign new address.
   * @param addr new address
   * @param addrlen specifies the size, in bytes, of the  address
                    structure pointed to by addr
   */
  virtual void Assign(const struct sockaddr* addr, socklen_t addrlen);
  virtual void Reset();
  [[nodiscard]] virtual std::unique_ptr<Address> Clone() const = 0;

  [[nodiscard]] uint16_t port() const;
  [[nodiscard]] virtual const struct sockaddr* sockaddr() const = 0;
  [[nodiscard]] virtual socklen_t socklen() const = 0;
private:
  uint16_t port_;
};

}

#endif // UDP_SERVER_NET_ADDRESS_H_
