#!/bin/bash

source env.sh

client_bin() {
 for release_or_debug in {release,debug}; do
	file="${CLIENT_BUILD_DIR}/${release_or_debug}/udp_client";
	[ -f "${file}" ] && echo $file && break;
 done;
}

get_files() {
 path=`readlink -f "${BASH_SOURCE:-$0}"`;
 dir_of_this_script=`dirname "${path}"`;
 dir_of_files="${dir_of_this_script}/files";
 echo "${dir_of_files}"/*;
}

run_client() {
 client=$(client_bin);
 ("${client}" --port $PORT $(get_files)) | xargs -I % echo "Client said: %";
}

"${SERVER}" $PORT &
server_pid=$!;

run_client;
run_client;

kill -s TERM $server_pid;
wait $server_pid;
