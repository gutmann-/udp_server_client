mod packet;
mod packets_view;
mod sender;
mod consts;

use crate::packets_view::{Packets, PacketsSource};
use crate::packet::Packet;
use crate::sender::PacketsSender;

use async_std::net::UdpSocket;

use clap::Parser;
use rand::{thread_rng, seq::SliceRandom};
use std::{
    cmp,
    fs::File,
    num::NonZeroU32,
    time::Duration,
};

use governor::Quota;
use crate::consts::*;


#[derive(Parser)]
#[command(author, version, about, long_about = None)]
struct Cli {
    #[arg(long, default_value = "localhost")]
    host: String,
    #[arg(long)]
    port: u16,
    #[arg(long, default_value_t = DEFAULT_SPEED_LIMIT.try_into().unwrap())]
    speed_limit: u32,
    #[arg(long, default_value_t = 1000)]
    timeout: u64,

    files: Vec<String>,
}

fn open_files(paths: &Vec<String>) -> Vec<PacketsSource> {
    paths
        .iter()
        .enumerate()
        .map(|(id, path)| match File::open(path) {
            Ok(file) => (path, file.to_packets_source(id as u64, MAX_PACKET_DATA_SIZE)),
            Err(e) => {
                println!("Can't open file '{}', error: {}", path, e);
                (path, Err(e))
            }
        })
        .filter_map(|(path, result)| match result {
            Ok(file) => Some(file),
            Err(e) => {
                println!("Can't create chunked view file for file '{}', error: {}", path, e);
                None
            },
        }).collect()
}

fn collect_packets(files: &'_ Vec<PacketsSource>) -> Vec<Packet<'_>> {
    files
        .iter()
        .flat_map(|file| file.packets())
        .collect::<Vec<Packet<'_>>>()
}

#[async_std::main]
async fn main() {
    let cli = Cli::parse();

    let files = open_files(&cli.files);

    let mut packets = collect_packets(&files);
    if packets.len() == 0 {
        return;
    }

    packets.shuffle(&mut thread_rng());

    let socket = UdpSocket::bind("0.0.0.0:0").await.unwrap();
    let connect_to_addr = format!("{host}:{port}", host = cli.host, port = cli.port);
    println!("Connecting to server {}...", connect_to_addr);
    socket.connect(connect_to_addr).await.unwrap();

    let speed_limit = cmp::max(cli.speed_limit, MAX_DATAGRAM_SIZE.try_into().unwrap());
    let quota = Quota::per_second(NonZeroU32::new(speed_limit).unwrap());
    let timeout = Duration::from_millis(cli.timeout);
    let sender = PacketsSender::new(packets, quota, timeout);
    sender.send(socket).await;
}