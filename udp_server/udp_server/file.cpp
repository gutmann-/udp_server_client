#include "udp_server/file.h"

#include "udp_server/packet.h"

namespace udp_server {

uint8_t File::ConstIterator::operator*() const {
  return *segment_iterator_;
}

File::ConstIterator& File::ConstIterator::operator++() {
  if (++segment_iterator_ == current_segment().end()) {
    if (current_segment_ < file_->capacity() - 1) {
      ++current_segment_;
      segment_iterator_ = current_segment().begin();
    }
  }

  return *this;
}

File::ConstIterator File::ConstIterator::operator++(int) {
  auto tmp = *this;
  ++(*this);
  return tmp;
}

bool File::ConstIterator::operator==(const ConstIterator& other) const {
  return file_             == other.file_            &&
         current_segment_  == other.current_segment_ &&
         segment_iterator_ == other.segment_iterator_ ;
}

bool File::ConstIterator::operator!=(const ConstIterator& other) const {
  return !(*this == other);
}

const std::vector<uint8_t>& File::ConstIterator::current_segment() const {
  return file_->segments_.at(current_segment_);
}

File::ConstIterator::ConstIterator(const File* file)
     : file_(file),
       current_segment_(0),
       segment_iterator_(current_segment().begin()) {}

File::File(uint64_t id, uint32_t number_of_segments)
     : id_(id),
       number_of_segments_(number_of_segments),
       segments_() {}

bool File::AddSegment(uint64_t file_id, uint32_t segment_no, std::vector<uint8_t>&& data) {
  if (file_id != id_ || segment_no >= number_of_segments_) return false;
  segments_.emplace(segment_no, data);
  return true;
}

bool File::AddSegment(Packet&& packet) {
  return AddSegment(packet.header().file_id, packet.header().seq_number, packet.TakeData());
}

File::ConstIterator File::begin() const { return ConstIterator(this); }
File::ConstIterator File::end() const {
  auto iterator = begin();
  iterator.current_segment_ = capacity() - 1;
  iterator.segment_iterator_ = segments_.at(capacity() - 1).end();
  return iterator;
}

} // namespace udp_server