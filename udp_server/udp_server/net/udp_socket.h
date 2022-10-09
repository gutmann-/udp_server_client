#ifndef UDP_SERVER_NET_UDP_SOCKET_H_
#define UDP_SERVER_NET_UDP_SOCKET_H_

#include "udp_server/net/socket.h"

namespace udp_server::net {

/**
 * Simple UDP socket.
 */
class UDPSocket : public Socket {
public:
  UDPSocket();

  /// Group of function for data receiving from clients
  /// This function works similar to recv/recvfrom function from
  /// POSIX
  /// @{
  ssize_t Recv(uint8_t* buf, size_t len, int flags);
  ssize_t Recv(std::vector<uint8_t>* to, int flags);

  std::pair<ssize_t, std::unique_ptr<Address>> RecvFrom(uint8_t* buf, size_t len, int flags);
  std::pair<ssize_t, std::unique_ptr<Address>> RecvFrom(std::vector<uint8_t>* to, int flags);
  /// }@

  /// Group of function to send data to specific address
  /// @param to address to send a data
  /// @param buf a data
  /// @param flags flags for sendto function
  /// @{
  ssize_t SendTo(const Address& to, const uint8_t* buf, size_t len, int flags);
  ssize_t SendTo(const Address& to, const std::vector<uint8_t>& what, int flags);
  /// @}
};

} // namespace udp_server::net

#endif // UDP_SERVER_NET_UDP_SOCKET_H_
