#include <sys/time.h> 

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

#include "common/ClockEstimator.hpp"
#include "common/Socket.hpp"
#include "common/SyncProbe.hpp"

std::vector<uint8_t> SerializeDouble(double value) {
  std::vector<uint8_t> buffer(sizeof(double));
  std::memcpy(buffer.data(), &value, sizeof(double));
  return buffer;
}

uint64_t GetCurrentTime() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

constexpr uint8_t PHASE_TRIGGER = 255;

class ServerSession : public INetworkLink {
  UDPServer& server;
  struct sockaddr_in client_addr;

 public:
  ServerSession(UDPServer& s, struct sockaddr_in addr)
      : server(s), client_addr(addr) {}

  void Send(const std::vector<uint8_t>& data) override {
    server.SendTo(data, client_addr);
  }

  std::vector<uint8_t> Receive(size_t max_bytes) override {
    struct sockaddr_in sender;
    return server.ReceiveFrom(max_bytes, sender);
  }
};

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: ./delay_server <Port>\n";
    return EXIT_FAILURE;
  }

  try {
    UDPServer server(static_cast<uint16_t>(std::stoi(argv[1])));

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(server.get_fd(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) <
        0) {
      std::cerr << "Warning: Failed to set socket timeout.\n";
    }

    std::cout << "Starting UDP Echo Server on port " << argv[1]
              << "...\n";

    while (true) {
      try {
        struct sockaddr_in client_addr{};
        bool trigger_received = false;

        while (!trigger_received) {
          auto data = server.ReceiveFrom(1024, client_addr);
          uint64_t rx_time = GetCurrentTime();

          if (data.size() >= SyncProbe::PAYLOAD_SIZE) {
            SyncProbe probe = SyncProbe::Deserialize(data);

            if (probe.sequence_number == PHASE_TRIGGER) {
              trigger_received = true;
              break;
            }

            probe.t_receive = rx_time;
            server.SendTo(probe.Serialize(), client_addr);
          }
        }

        std::cout << "\nTrigger received. Probing backward path...\n";
        ServerSession session(server, client_addr);

        ClockEstimator estimator(session);
        double server_offset = estimator.CalculateOffset();

        std::cout << "Sending calculated offset (" << server_offset
                  << ") to client...\n";
        server.SendTo(SerializeDouble(server_offset), client_addr);
        std::cout << "Complete. Resetting state.\n";

      } catch (const std::system_error& e) {
        if (e.code().value() == EAGAIN || e.code().value() == EWOULDBLOCK) {
          continue;
        } else {
          std::cerr << "[Network Error] " << e.what() << "\n";
        }
      } catch (const std::exception& e) {
        std::cerr << "[Session Failed] " << e.what() << ". Dropping client.\n";
      }
    }

  } catch (const std::exception& e) {
    std::cerr << "Fatal Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}