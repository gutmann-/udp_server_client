#!/bin/bash

source env.sh

build_server() {
 cmake -DCMAKE_BUILD_TYPE=Release \
			 -DCMAKE_MAKE_PROGRAM=ninja \
			 -DCMAKE_C_COMPILER=gcc \
			 -DCMAKE_CXX_COMPILER=g++ \
			 -G Ninja \
			 -S udp_server \
			 -B "${SERVER_BUILD_DIR}";

 cmake --build "${SERVER_BUILD_DIR}" --target udp_server -j 4;
}

build_client() {
 cwd=`pwd`;
 cd udp_client;
 CARGO_TARGET_DIR="${CLIENT_BUILD_DIR}" cargo build --release;
 cd "${cwd}";
}

build_server && build_client;
