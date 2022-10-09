#include <csignal>
#include <filesystem>
#include <iostream>

#include "udp_server/net/ipv4_address.h"
#include "udp_server/net/udp_socket.h"
#include "udp_server/server.h"


void HandleIntSignal(int signal) {
  if (signal == SIGTERM)
    std::cout << "Exiting..." << std::endl;
  else
    std::cerr << "Unexpected signal #" << signal << std::endl;
}

bool SetupSignals() {
  struct sigaction sigbreak = {};
  sigbreak.sa_handler = &HandleIntSignal;
  sigbreak.sa_flags = 0;
  sigemptyset(&sigbreak.sa_mask);

  const auto success = sigaction(SIGTERM, &sigbreak, nullptr) == 0;
  if (!success) {
    std::perror("sigaction");
  }

  return success;
}

int RunServer(int port) {
  using namespace udp_server;

  net::UDPSocket socket;
  const auto success = socket.Bind(std::make_unique<net::IPv4Address>(port));
  if (success) {
    Server server(std::move(socket));

    server.OnNewFile([](const File& file, uint32_t crc32) {
      std::cout << "Got new file with id == " << file.id()
                << " and crc32 == " << crc32 << std::endl;
    });

    server.Run();
    return 0;
  } else {
    std::cerr << "Can't bind socket to port #" << port << std::endl;
    return 1;
  }
}

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    const std::filesystem::path this_executable_path(argv[0]);
    std::cerr << "Usage: " << this_executable_path.filename().string() << " PORT" << std::endl;
    return 1;
  }

  if (!SetupSignals()) {
    return 1;
  }

  const auto port = std::stoi(argv[1]);
  std::cout << "Running server on port #" << port << std::endl;
  const auto exit_code = RunServer(port);
  std::cout << "Done!" << std::endl << "Exit code == " << exit_code << std::endl;

  return exit_code;
}
