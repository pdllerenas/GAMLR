#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

#include "common/Socket.hpp"
#include "common/SyncProbe.hpp"

uint64_t GetCurrentTime() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: ./delay_server <Port>\n";
    return EXIT_FAILURE;
  }

  uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));

  try {
    UDPServer server(port);
    std::cout << "Starting UDP Echo Server on port " << port << "...\n";

    while (true) {
      struct sockaddr_in client_addr{};

      std::vector<uint8_t> data = server.ReceiveFrom(1024, client_addr);
      uint64_t rx_time = GetCurrentTime();

      if (data.size() >= 17) {
        SyncProbe probe = SyncProbe::Deserialize(data);
        probe.t_receive = rx_time;
        server.SendTo(probe.Serialize(), client_addr);
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}