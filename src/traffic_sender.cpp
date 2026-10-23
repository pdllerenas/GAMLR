#include "common/Types.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " <IP> <PORT> <GAP_US>\n";
    return 1;
  }

  const char *ip = argv[1];
  int port = std::stoi(argv[2]);
  int gap_us = std::stoi(argv[3]);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  inet_pton(AF_INET, ip, &server_addr.sin_addr);

  char buffer[PAYLOAD_SIZE] = {(char)255};
  PacketHeader *header = reinterpret_cast<PacketHeader *>(buffer);

  uint32_t train_id = 0;

  while (true) {
    std::cout << "Sending Train " << train_id << " (Gap: " << gap_us << "us)\n";

    // Send K packets paced by the specified microsecond gap
    for (uint32_t i = 0; i < K_TRAIN_LENGTH; ++i) {
      header->train_id = train_id;
      header->seq_num = i;

      // Capture the exact time right before sending
      auto send_time = std::chrono::steady_clock::now();

      sendto(sockfd, buffer, PAYLOAD_SIZE, 0, (struct sockaddr *)&server_addr,
             sizeof(server_addr));

      // Precise busy-wait to enforce the inter-packet gap
      if (gap_us > 0 && i < (K_TRAIN_LENGTH - 1)) {
        while (true) {
          auto now = std::chrono::steady_clock::now();
          auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                             now - send_time)
                             .count();
          if (elapsed >= gap_us) {
            break;
          }
        }
      }
    }

    train_id++;
    usleep(500000); // 500ms pause between trains
  }

  close(sockfd);
  return 0;
}
