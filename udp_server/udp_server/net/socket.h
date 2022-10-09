#ifndef UDP_SERVER_NET_SOCKET_H_
#define UDP_SERVER_NET_SOCKET_H_

#include "udp_server/net/address.h"

#include <memory>

namespace udp_server::net {

/**
 * Base class for socket. TCP or UDP or other kind of a socket.
 */
class Socket {
public:
  static const int BAD_FD;

  Socket(int domain, int type, int protocol);
  Socket(const Socket&) = delete;
  Socket(Socket&& from) noexcept;
  ~Socket();

  Socket& operator=(const Socket&) = delete;
  Socket& operator=(Socket&& from) noexcept;

  bool Bind(std::unique_ptr<Address> to);

  [[nodiscard]] const Address* bound_to() const { return bound_to_.get(); }
protected:
  [[nodiscard]] int socket_fd() const { return fd_; }
private:
  int fd_;
  std::unique_ptr<Address> bound_to_;
};

} // namespace udp_server::net

#endif // UDP_SERVER_NET_SOCKET_H_
