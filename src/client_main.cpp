#include <cstdlib>
#include <iostream>
#include <string>

#include "common/ClockEstimator.hpp"
#include "common/Socket.hpp"
#include "common/SyncProbe.hpp"

constexpr uint8_t PHASE_TRIGGER = 255;

double DeserializeDouble(const std::vector<uint8_t>& data) {
  if (data.size() < sizeof(double)) {
    throw std::runtime_error(
        "Malformed packet: insufficient bytes for double.");
  }
  double value;
  std::memcpy(&value, data.data(), sizeof(double));
  return value;
}

uint64_t GetCurrentTimeClient() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: ./delay_client <Server_IP> <Port> [Packet Size]\n";
    return EXIT_FAILURE;
  }

  std::string ip = argv[1];
  uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
  size_t packet_size = (argc >= 4) ? std::stoul(argv[3]) : 48;

  try {
    std::cout << "Connecting to Server at " << ip << ":" << port << "...\n";
    UDPClient client(ip, port);
    ClockEstimator estimator(client, packet_size);

    std::cout << "Probing forward path...\n";
    double local_offset = estimator.CalculateOffset();

    std::cout << "Server probing...\n";
    SyncProbe trigger{PHASE_TRIGGER, 0, 0};
    client.Send(trigger.Serialize(packet_size));

    std::cout << "Echoing Server probes...\n";
    double server_offset = 0.0;

    while (true) {
        auto data = client.Receive(65536);

        if (data.size() == sizeof(double)) {
            server_offset = DeserializeDouble(data);
            break;
        } else if (data.size() >= SyncProbe::PAYLOAD_SIZE) {
            SyncProbe probe = SyncProbe::Deserialize(data);
            probe.t_receive = GetCurrentTimeClient();
            client.Send(probe.Serialize(data.size()));
        }
    }

    double collaborative_offset = (server_offset - local_offset) / 2.0;

    std::cout << "-----------------------------------\n";
    std::cout << "Local Offset (gamma):  " << local_offset << " ms\n";
    std::cout << "Server Offset (gamma): " << server_offset << " ms\n";
    std::cout << "Final Symmetrical Offset: " << collaborative_offset
              << " ms\n";
    std::cout << "-----------------------------------\n";

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
