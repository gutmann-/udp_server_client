use crate::packet::Header;

use bytesize;

pub const MAX_DATAGRAM_SIZE: usize = 1472;
pub const MAX_PACKET_DATA_SIZE: usize = MAX_DATAGRAM_SIZE - Header::serialized_size();
pub const DEFAULT_SPEED_LIMIT: u64 = 10 * bytesize::MIB;