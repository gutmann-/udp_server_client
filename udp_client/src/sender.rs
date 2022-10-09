use async_std::{net::UdpSocket, task::yield_now};
use futures::{
    stream::iter,
    future::{Abortable, AbortHandle, join},
    StreamExt,
};
use std::{
    cell::RefCell,
    collections::{HashMap, HashSet},
    num::NonZeroU32,
};
use itertools::Itertools;

use governor::{
    clock::DefaultClock,
    state::{NotKeyed, InMemoryState},
    Quota, RateLimiter,
};

use crate::consts;
use crate::packet::{Data, Packet, Crc32Sum, EncodeToVec};

fn calc_hashes_for_files(packets: &Vec<Packet<'_>>) -> HashMap<u64, u32> {
    let mut ref_to_packets_vec = packets.iter().collect::<Vec<&Packet<'_>>>();
    ref_to_packets_vec.sort_by(|a, b| a.header.file_id.cmp(&b.header.file_id));

    ref_to_packets_vec
        .into_iter()
        .group_by(|packet| packet.header.file_id)
        .into_iter()
        .map(|(file_id, packets)| {
            let mut packets = packets.collect::<Vec<&Packet<'_>>>();
            packets.sort_by(|a, b| a.header.seq_number.cmp(&b.header.seq_number));
            (file_id, packets.crc32())
        })
        .collect::<HashMap<_, _>>()
}

pub struct PacketsSender<'a> {
    packets: Vec<Packet<'a>>,
    crc32: HashMap<u64, u32>,
    received_crc32: RefCell<HashMap<u64, u32>>, // file_id => crc32
    acks: RefCell<HashSet<(u64, u32)>>,
    rate_limiter: RateLimiter<NotKeyed, InMemoryState, DefaultClock>,
}

impl<'a> PacketsSender<'a> {
    pub fn new(packets: Vec<Packet<'a>>, quota: Quota) -> Self {
        let crc32_map = calc_hashes_for_files(&packets);
        Self {
            packets,
            crc32: crc32_map,
            received_crc32: RefCell::new(HashMap::new()),
            acks: RefCell::new(HashSet::new()),
            rate_limiter: RateLimiter::direct(quota),
        }
    }

    pub async fn send(&self, socket: UdpSocket) {
        let (send_task_abort_handle, send_task_abort_reg) = AbortHandle::new_pair();

        let send_task = iter(self.packets.iter())
            .for_each(|packet| async {
                if let Err(e) = self.send_packet(packet, &socket).await {
                    println!("Error while sending packet! Error: {}", e);
                    send_task_abort_handle.abort();
                }
            });

        let send_task = Abortable::new(send_task, send_task_abort_reg);
        let (_, recv_result) = join(send_task, self.receive(&socket)).await;
        if let Err(e) = recv_result {
            println!("Error while receiving packet! Error: {}", e);
        }
    }

    async fn send_packet(
        &self,
        packet: &Packet<'_>,
        socket: &UdpSocket
    ) -> Result<(), async_std::io::Error> {
        let file_id = &packet.header.file_id;
        let binary_packet = packet.encode_to_vec().unwrap();
        let binary_packet_size =
            NonZeroU32::new(binary_packet.len().try_into().unwrap()).unwrap();

        while !self.upload_is_complete(file_id) && !self.is_ack_received_for_packet(&packet) {
            self.rate_limiter.until_n_ready(binary_packet_size).await.unwrap();
            socket.send(&binary_packet).await?;
            yield_now().await;
        }

        Ok(())
    }

    async fn receive(&self, socket: &UdpSocket) -> Result<(), async_std::io::Error> {
        while self.received_crc32.borrow().len() < self.crc32.len() {
            let mut buf = vec![0; consts::MAX_DATAGRAM_SIZE];
            let bytes_received = socket.recv(&mut buf).await?;
            let packet = match Packet::decode_from_slice(&buf[..bytes_received]) {
                Err(_) => { println!("Can't decode packet"); continue; },
                Ok(packet) => packet,
            };

            let k = (packet.header.file_id.clone(), packet.header.seq_number.clone());
            self.acks.borrow_mut().insert(k);

            if let Data::Crs32(crc32) = packet.data {
                let file_id = &packet.header.file_id;
                let old = self.received_crc32.borrow_mut().insert(*file_id, crc32.clone());

                let to_print = match old {
                    Some(old_crc32) if old_crc32 != crc32 => true,
                    None => true,
                    Some(_) => false,
                };

                if to_print {
                    println!("file_id == {}, calculated crc == {}, received_crc == {}",
                             file_id,
                             self.crc32.get(&file_id).unwrap_or(&0),
                             crc32);
                }
            }
        }

        Ok(())
    }

    fn upload_is_complete(&self, file_id: &u64) -> bool {
        match (self.crc32.get(&file_id), self.received_crc32.borrow().get(&file_id)) {
            (Some(expected_crc), Some(received_crc)) => received_crc == expected_crc,
            (_, _) => false,
        }
    }

    fn is_ack_received_for_packet(&self, packet: &Packet<'_>) -> bool {
        let k = (packet.header.file_id.clone(), packet.header.seq_number.clone());
        self.acks.borrow().contains(&k)
    }
}