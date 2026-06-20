#include <cstdlib>
#include <iostream>
#include <string>

#include "common/ClockEstimator.hpp"
#include "common/Socket.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: ./delay_client <Server_IP> <Port>\n";
    return EXIT_FAILURE;
  }

  std::string ip = argv[1];
  uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

  try {
    std::cout << "Connecting to Server at " << ip << ":" << port << "...\n";
    UDPClient client(ip, port);
    ClockEstimator estimator(client);

    std::cout << "Initiating Probe Burst...\n";
    double final_offset = estimator.CalculateOffset();

    std::cout << "-----------------------------------\n";
    std::cout << "Final Estimated Offset: " << final_offset << " ms\n";
    std::cout << "-----------------------------------\n";
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}