use crc::{Crc, CRC_32_ISCSI};
use bincode::{
    serde::{decode_from_slice},
    error::{DecodeError, EncodeError},
};
use serde::{Deserialize, Serialize};
use serde_repr::{Deserialize_repr, Serialize_repr};
use std::{assert_eq, panic, mem::size_of};

#[derive(Clone, Serialize_repr, Deserialize_repr, Debug)]
#[repr(u8)]
pub enum PacketType {
    ACK = 0,
    PUT = 1,
    UNKNOWN = 0xff,
}

#[derive(Clone, Serialize, Deserialize, Debug)]
pub struct Header {
    pub seq_number: u32,
    pub seq_total: u32,
    pub type_: PacketType,
    pub file_id: u64,
}

impl Header {
    pub const fn serialized_size() -> usize {
        size_of::<u32>() +
            size_of::<u32>() +
            size_of::<u8>() +
            size_of::<u64>()
    }
}

pub enum Data<'a> {
    Ref(&'a [u8]),
    Copy(Vec<u8>),
    Crs32(u32),
    Empty,
}

pub struct Packet<'a> {
    pub header: Header,
    pub data: Data<'a>,
}

impl Packet<'_> {
    pub fn decode_from_slice(slice: &[u8]) -> Result<Self, DecodeError> {
        let config = bincode::config::standard()
            .with_big_endian()
            .with_fixed_int_encoding();

        let (header, bytes_decoded) =
            decode_from_slice::<Header, _>(&slice, config.clone())?;

        let data = match header.type_ {
            PacketType::ACK => {
                let have_crc32_in_data = (slice.len() - bytes_decoded) >= size_of::<u32>();
                if have_crc32_in_data {
                    let (crc32, _) = decode_from_slice::<u32, _>(&slice[bytes_decoded..],
                                                                 config.clone())?;
                    Data::Crs32(crc32)
                } else {
                    Data::Empty
                }
            }
            PacketType::PUT => {
                let mut data = Vec::new();
                data.extend_from_slice(&slice[bytes_decoded..]);
                Data::Copy(data)
            }
            PacketType::UNKNOWN => {
                panic!("UNKNOWN Packet");
            }
        };

        Ok(Self { header, data })
    }
}

pub trait EncodeToVec {
    fn encode_to_vec(&self) -> Result<Vec<u8>, EncodeError>;
}

impl EncodeToVec for Packet<'_> {
    fn encode_to_vec(&self) -> Result<Vec<u8>, EncodeError> {
        let config = bincode::config::standard()
            .with_big_endian()
            .with_fixed_int_encoding();

        let mut vec_ = bincode::serde::encode_to_vec(&self.header, config.clone())?;
        assert_eq!(vec_.len(), Header::serialized_size());
        match &self.data {
            Data::Ref(r) => vec_.extend_from_slice(r),
            Data::Copy(c) => vec_.extend_from_slice(c.as_slice()),
            Data::Crs32(crc32) => {
                let slice = bincode::serde::encode_to_vec(&crc32, config.clone())?;
                vec_.extend_from_slice(slice.as_slice());
            }
            Data::Empty => {},
        };

        Ok(vec_)
    }
}

pub trait Crc32Sum {
    fn crc32(&self) -> u32;
}

impl Crc32Sum for Vec<&Packet<'_>> {
    fn crc32(&self) -> u32 {
        let crc_calculator = Crc::<u32>::new(&CRC_32_ISCSI);
        let mut digest = crc_calculator.digest();
        self.iter().for_each(|packet| {
            match &packet.data {
                Data::Ref(slice) => digest.update(*slice),
                Data::Copy(vec) => digest.update(vec.as_slice()),
                Data::Crs32(_) | Data::Empty => {},
            }
        });

        digest.finalize()
    }
}

impl Crc32Sum for Vec<Packet<'_>> {
    fn crc32(&self) -> u32 {
        self.iter().collect::<Vec<&Packet<'_>>>().crc32()
    }
}