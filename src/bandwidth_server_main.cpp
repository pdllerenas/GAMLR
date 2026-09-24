#include <iostream>

#include "common/Socket.hpp"
#include "common/SyncProbe.hpp"
#include "common/Types.hpp"

uint64_t GetCurrentTime() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

double AvailableBandwidth(double C, double D_out, double D_in) {
  return C * (1 - (D_out - D_in) / D_in);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: ./avbw_server <Port>\n";
    return EXIT_FAILURE;
  }

  uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));
  UDPServer server(port);

  bool has_first_packet = false;
  SyncProbe probe1;
  uint64_t rx_time_1 = 0;

  constexpr double LINK_CAPACITY_MBPS = 50.0;
  std::cout << "Listening for packet pairs on port " << port << "...\n";

  // process packets by pairs
  while (true) {
    struct sockaddr_in client_addr{};

    auto data = server.ReceiveFrom(65536, client_addr);
    uint64_t rx_time = GetCurrentTime();

    SyncProbe current_probe = SyncProbe::Deserialize(data);

    if (current_probe.sequence_number % 2 == 0) {
      probe1 = current_probe;
      rx_time_1 = rx_time;
      has_first_packet = true;
    } else if (has_first_packet &&
               current_probe.sequence_number == probe1.sequence_number + 1) {
      uint64_t rx_time_2 = rx_time;
      double D_out = static_cast<double>(rx_time_2 - rx_time_1);
      double D_in = static_cast<double>(current_probe.t_send - probe1.t_send);

      if (D_in > 0) {
        double abw = AvailableBandwidth(LINK_CAPACITY_MBPS, D_out, D_in);

        std::cout << "Pair [" << probe1.sequence_number << ", "
                  << current_probe.sequence_number << "] | D_in: " << D_in
                  << " us | D_out: " << D_out << " us | Est. ABW: " << abw
                  << " Mbps\n";
      } else {
        std::cerr << "Invalid D_in\n";
      }
      has_first_packet = false;
    } else {
      std::cerr << "Packet lost or reordering detected.\n";
      has_first_packet = false;
    }
  }
}
