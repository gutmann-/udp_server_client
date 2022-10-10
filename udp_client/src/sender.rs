use async_std::{channel, future, net::UdpSocket};
use futures::{
    future::{join, AbortHandle, Abortable, Aborted},
    stream::iter,
    StreamExt,
};
use itertools::Itertools;
use std::{cell::RefCell, collections::HashMap, num::NonZeroU32, time::Duration};

use governor::{
    clock::DefaultClock,
    state::{InMemoryState, NotKeyed},
    Quota, RateLimiter,
};

use crate::consts;
use crate::packet::{Crc32Sum, Data, EncodeToVec, Packet};

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
    crc32: HashMap<u64, u32>,
    received_crc32: RefCell<HashMap<u64, u32>>, // file_id => crc32
    rate_limiter: RateLimiter<NotKeyed, InMemoryState, DefaultClock>,
    senders: HashMap<(u64, u32), OnePacketSender<'a>>,
    timeout: Duration,
    number_of_sent_packets: RefCell<usize>,
}

impl<'a> PacketsSender<'a> {
    pub fn new(packets: Vec<Packet<'a>>, quota: Quota, timeout: Duration) -> Self {
        Self {
            crc32: calc_hashes_for_files(&packets),
            received_crc32: RefCell::new(HashMap::new()),
            rate_limiter: RateLimiter::direct(quota),
            senders: packets
                .into_iter()
                .map(|packet| {
                    let k = (packet.header.file_id, packet.header.seq_number);
                    (k, OnePacketSender::new(packet))
                })
                .collect::<HashMap<_, OnePacketSender<'a>>>(),

            timeout,
            number_of_sent_packets: RefCell::new(0),
        }
    }

    pub async fn send(&self, socket: UdpSocket) {
        let (send_task_abort_handle, send_task_abort_reg) = AbortHandle::new_pair();
        let (recv_task_abort_handle, recv_task_abort_reg) = AbortHandle::new_pair();

        let send_task = iter(self.senders.values()).for_each(|sender| async {
            if let Err(e) = sender.send(&socket, &self.rate_limiter, self.timeout).await {
                println!("Error while sending packet! Error: {}", e);
                send_task_abort_handle.abort();
                recv_task_abort_handle.abort()
            }

            self.number_of_sent_packets.replace_with(|&mut old| old + 1);
            let last_packet_sent = *self.number_of_sent_packets.borrow() == self.senders.len();

            if last_packet_sent {
                recv_task_abort_handle.abort();
            }
        });

        let send_task = Abortable::new(send_task, send_task_abort_reg);
        let recv_task = Abortable::new(self.receive(&socket), recv_task_abort_reg);

        let (_, recv_result) = join(send_task, recv_task).await;
        match recv_result {
            Ok(finished) => {
                if let Err(e) = finished {
                    println!("Error while receiving packet! Error: {}", e);
                }
            }
            Err(Aborted) => {}
        }
    }

    async fn receive(&self, socket: &UdpSocket) -> Result<(), async_std::io::Error> {
        loop {
            let mut buf = vec![0; consts::MAX_DATAGRAM_SIZE];
            let bytes_received = socket.recv(&mut buf).await?;
            let packet = match Packet::decode_from_slice(&buf[..bytes_received]) {
                Err(_) => {
                    println!("Can't decode packet");
                    continue;
                }
                Ok(packet) => packet,
            };

            if let Data::Crs32(crc32) = packet.data {
                let file_id = &packet.header.file_id;
                let old = self
                    .received_crc32
                    .borrow_mut()
                    .insert(*file_id, crc32.clone());

                let to_print = match old {
                    Some(old_crc32) if old_crc32 != crc32 => true,
                    None => true,
                    Some(_) => false,
                };

                if to_print {
                    println!(
                        "file_id == {}, calculated crc == {}, received_crc == {}",
                        file_id,
                        self.crc32.get(&file_id).unwrap_or(&0),
                        crc32
                    );
                }
            }

            let k = (
                packet.header.file_id.clone(),
                packet.header.seq_number.clone(),
            );
            if let Some(sender) = self.senders.get(&k) {
                sender.ack(packet).await;
            }
        }
    }
}

struct OnePacketSender<'a> {
    packet: Packet<'a>,

    s: channel::Sender<Packet<'a>>,
    r: channel::Receiver<Packet<'a>>,
}

impl<'a> OnePacketSender<'a> {
    fn new(packet: Packet<'a>) -> Self {
        let (s, r) = channel::bounded::<Packet<'a>>(1);
        Self { packet, s, r }
    }

    async fn send(
        &self,
        socket: &UdpSocket,
        rate_limiter: &RateLimiter<NotKeyed, InMemoryState, DefaultClock>,
        timeout: Duration,
    ) -> Result<(), async_std::io::Error> {
        let binary_packet = self.packet.encode_to_vec().unwrap();
        let binary_packet_size = NonZeroU32::new(binary_packet.len().try_into().unwrap()).unwrap();

        loop {
            rate_limiter
                .until_n_ready(binary_packet_size)
                .await
                .unwrap();
            socket.send(&binary_packet).await?;

            let ack_packet = match future::timeout(timeout, self.r.recv()).await {
                Ok(recv_result) => match recv_result {
                    Ok(packet) => packet,
                    Err(_) => {
                        println!("Channel is closed!");
                        break;
                    }
                },
                Err(_) => {
                    continue;
                } // timeout
            };

            let ack_packet_k = (ack_packet.header.file_id, ack_packet.header.seq_number);
            let this_packet_k = (self.packet.header.file_id, self.packet.header.seq_number);
            if ack_packet_k == this_packet_k {
                self.s.close();
                break;
            } else {
                println!("Warning! Wrong ack packet!");
            }
        }

        Ok(())
    }

    async fn ack(&self, packet: Packet<'a>) {
        // udp packets can have a duplicate so we ignore
        // send error for channel::Sender::send method
        // because it returns an error only if it was closed...
        // (...and we closing self.s in self.send)
        let _ = self.s.send(packet).await;
    }
}
