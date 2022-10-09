#ifndef UDP_SERVER_BASE_SYS_BYTEORDER_H_
#define UDP_SERVER_BASE_SYS_BYTEORDER_H_

#include <bit>

namespace udp_server::base {

inline uint32_t HostToNet32(uint32_t x) {
  if constexpr (std::endian::native == std::endian::little) {
    return __builtin_bswap32(x);
  } else
    return x;
}

inline uint64_t HostToNet64(uint64_t x) {
  if constexpr (std::endian::native == std::endian::little) {
    return __builtin_bswap64(x);
  } else
    return x;
}

} // namespace udp_server::base

#endif // UDP_SERVER_BASE_SYS_BYTEORDER_H_
