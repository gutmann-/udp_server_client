#ifndef UDP_SERVER_BASE_CRC32_H_
#define UDP_SERVER_BASE_CRC32_H_

#include <bits/stdint-uintn.h>

namespace udp_server::base {

template <class Iterator>
uint32_t Crc32(uint32_t crc, Iterator begin, Iterator end) {
  crc = ~crc;

  while (begin != end) {
    crc ^= *begin++;
    for (int k = 0; k < 8; k++)
      crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1;
  }

  return ~crc;
}

template <class Iterator>
uint32_t Crc32(Iterator begin, Iterator end) {
  return Crc32(0, begin, end);
}

} // namespace udp_server::base

#endif // UDP_SERVER_BASE_CRC32_H_
