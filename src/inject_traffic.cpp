#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "common/Socket.hpp"
#include "common/SyncProbe.hpp"

int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cerr << "Usage: " << argv[0]
              << " <IP_Address> <Port> <Rate_Hz> <Packet_Size>\n"
              << "Example: " << argv[0] << " 127.0.0.1 8080 10 512\n";
    return 1;
  }

  std::string ip = argv[1];
  uint16_t port;
  int rate_hz;
  size_t packet_size;

  try {
    port = std::stoi(argv[2]);
    rate_hz = std::stoi(argv[3]);
    packet_size = std::stoul(argv[4]);
  } catch (const std::exception &e) {
    std::cerr
        << "Error: Invalid numeric format for port, rate, or packet size.\n";
    return 1;
  }

  if (rate_hz <= 0) {
    std::cerr << "Error: Rate must be greater than 0 Hz.\n";
    return 1;
  }

  auto delay = std::chrono::microseconds(1'000'000 / rate_hz);

  try {
    UDPClient client(ip, port);
    uint8_t seq_num = 0;

    std::cout << "Sending " << packet_size << "-byte packets to " << ip << ":"
              << port << " at " << rate_hz << " Hz...\n";

    auto next_send_time = std::chrono::steady_clock::now();

    while (true) {
      auto now = std::chrono::system_clock::now();
      uint64_t t_send = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            now.time_since_epoch())
                            .count();

			// Form the packet
      SyncProbe probe{seq_num, t_send, 0};
      std::vector<uint8_t> payload = probe.Serialize(packet_size);

      client.Send(payload);

      seq_num++;

			// Sleep Thread
      next_send_time += delay;
      std::this_thread::sleep_until(next_send_time);
    }
  } catch (const std::exception &e) {
    std::cerr << "Network Error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
