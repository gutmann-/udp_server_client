#ifndef UDP_SERVER_FILE_H_
#define UDP_SERVER_FILE_H_

#include <bits/stdint-uintn.h>
#include <unordered_map>
#include <vector>

namespace udp_server {

class Packet;

/**
 * In-memory file which client can send to server.
 */
class File {
public:
  class ConstIterator {
  friend class File;
  public:
    uint8_t operator*() const;

    ConstIterator& operator++();
    ConstIterator operator++(int);

    bool operator==(const ConstIterator& other) const;
    bool operator!=(const ConstIterator& other) const;
  private:
    explicit ConstIterator(const File* file);

    [[nodiscard]] const std::vector<uint8_t>& current_segment() const;

    const File* const file_;
    size_t current_segment_;
    std::vector<uint8_t>::const_iterator segment_iterator_;
  };


  File(uint64_t id, uint32_t number_of_segments);

  bool AddSegment(uint64_t file_id, uint32_t segment_no, std::vector<uint8_t>&& data);
  bool AddSegment(Packet&& packet);

  uint64_t id() const { return id_; }

  /// @return current number of segments in this file
  size_t size() const { return segments_.size(); }
  /// @return number of segments in full file
  size_t capacity() const { return number_of_segments_; }
  /// @return file contains all necessary segments?
  bool full() const { return size() >= capacity(); }

  ConstIterator begin() const;
  ConstIterator end() const;
private:
  const uint64_t id_;
  const uint32_t number_of_segments_;

  /// Contains data segments which addressed by its `seq_number` value.
  /// You can reconstruct entire file if you have all segments in this map
  /// with seq_number in [0..=number_of_segments_-1] i.e. iterate int i
  /// from 0 to (number_of_segments_ - 1) and get each segment in segments_[i].
  /// You could do so iff you have all segments ot the file.
  std::unordered_map<uint32_t, std::vector<uint8_t>> segments_;
};

} // namespace udp_server

#endif // UDP_SERVER_FILE_H_
