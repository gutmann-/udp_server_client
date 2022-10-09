use crate::packet::{Data, Header, Packet, PacketType};
use memmap::Mmap;
use std::fs::File;

pub struct PacketsSource {
    id: u64,
    mmap: Mmap,
    packet_size: usize,
}

impl PacketsSource {
    pub fn new(id: u64, packet_size: usize, file: &File) -> Result<Self, std::io::Error> {
        Ok(PacketsSource {
            id,
            mmap: unsafe { Mmap::map(&file)? },
            packet_size,
        })
    }

    pub fn packets(&self) -> impl Iterator<Item = Packet> {
        let chunks = self.mmap.chunks(self.packet_size);
        let num_of_chunks = chunks.len();
        let id = self.id.clone();

        chunks
            .enumerate()
            .map(move |(idx, chunk)|
                Packet {
                    header: Header {
                        seq_number: idx.try_into().unwrap(),
                        seq_total: num_of_chunks.try_into().unwrap(),
                        type_: PacketType::PUT,
                        file_id: id,
                    },
                    data: Data::Ref(chunk),
                }
            )
    }
}

pub trait Packets {
    fn to_packets_source(
        &self,
        id: u64,
        packet_size: usize,
    ) -> Result<PacketsSource, std::io::Error>;
}

impl Packets for File {
    fn to_packets_source(
        &self,
        id: u64,
        packet_size: usize,
    ) -> Result<PacketsSource, std::io::Error> {
        PacketsSource::new(id, packet_size, &self)
    }
}