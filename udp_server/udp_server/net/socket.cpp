#include "udp_server/net/socket.h"

#include "udp_server/net/address.h"

#include <sys/socket.h>
#include <unistd.h>

namespace udp_server::net {

// static
const int Socket::BAD_FD = -1;

Socket::Socket(int domain, int type, int protocol)
       : fd_(socket(domain, type, protocol)) {}

Socket::Socket(Socket&& from) noexcept
       : fd_(from.fd_),
         bound_to_(std::move(from.bound_to_)) { from.fd_ = BAD_FD; }

Socket::~Socket() {
  if (fd_ != BAD_FD)
    close(fd_);
}

Socket& Socket::operator=(Socket&& from) noexcept {
  if (&from != this) {
    fd_ = from.fd_;
    from.fd_ = BAD_FD;

    bound_to_ = std::move(from.bound_to_);
  }

  return *this;
}

bool Socket::Bind(std::unique_ptr<Address> to) {
  const auto success = bind(fd_, to->sockaddr(), to->socklen()) == 0;
  if (success)
    bound_to_ = std::move(to);
  return success;
}

} // namespace udp_server::net