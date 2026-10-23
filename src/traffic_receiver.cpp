#include "common/Types.hpp"

#include <algorithm>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <PORT>\n";
    return 1;
  }

  int port = std::stoi(argv[1]);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);
  bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

  int opt = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_TIMESTAMP, &opt, sizeof(opt));

  char packet_buf[PAYLOAD_SIZE];
  char control_buf[1024];

  struct msghdr msg{};
  struct iovec iov[1];
  iov[0].iov_base = packet_buf;
  iov[0].iov_len = sizeof(packet_buf);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_control = control_buf;
  msg.msg_controllen = sizeof(control_buf);

  uint64_t t1 = 0;

  std::vector<bool> received_packets(K_TRAIN_LENGTH, false);
  uint32_t received_count = 0;

  // Track the current train to ignore duplicates and old packets
  uint32_t current_train_id = 0;
  bool is_first_packet = true;

  while (true) {
    ssize_t recv_len = recvmsg(sockfd, &msg, 0);
    if (recv_len < 0)
      continue;

    uint64_t recv_timestamp_us = 0;
    for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr;
         cmsg = CMSG_NXTHDR(&msg, cmsg)) {
      if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMP) {
        struct timeval *tv = (struct timeval *)CMSG_DATA(cmsg);
        recv_timestamp_us = tv->tv_sec * 1000000ULL + tv->tv_usec;
        break;
      }
    }

    PacketHeader *header = reinterpret_cast<PacketHeader *>(packet_buf);
    uint32_t seq = header->seq_num;
    uint32_t incoming_train = header->train_id;

    if (seq >= K_TRAIN_LENGTH) {
      continue;
    }

    // Bootstrap the very first train ID we see
    if (is_first_packet) {
      current_train_id = incoming_train;
      is_first_packet = false;
    }

    // Handle out-of-order trains
    if (incoming_train < current_train_id) {
      // This is a stale, delayed packet from a train we already finished or
      // abandoned. Ignore it.
      continue;
    }

    if (incoming_train > current_train_id) {
      // A newer train arrived before we finished the current one. The old train
      // lost packets.
      std::cout << "Train " << current_train_id << ": Packets Lost\n";

      // Reset state and lock onto the new train
      current_train_id = incoming_train;
      std::fill(received_packets.begin(), received_packets.end(), false);
      received_count = 0;
    }

    // At this point, incoming_train == current_train_id
    if (received_packets[seq]) {
      // We already have this sequence number for the current train. It is a
      // network duplicate. Ignore it so it doesn't corrupt the count.
      continue;
    }

    received_packets[seq] = true;
    received_count++;

    if (received_count == 1) {
      t1 = recv_timestamp_us;
    }

    if (received_count == K_TRAIN_LENGTH) {
      std::cout << "Train " << current_train_id << ": Received All Packets ("
                << K_TRAIN_LENGTH << ")\n";

      // Advance the expected train ID so duplicate packets from THIS train are
      // safely dropped
      current_train_id++;
      std::fill(received_packets.begin(), received_packets.end(), false);
      received_count = 0;
    }
  }
  return 0;
}
