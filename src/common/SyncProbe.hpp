#pragma once

#include <endian.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

struct SyncProbe {
  uint8_t sequence_number;
  uint64_t t_send;
  uint64_t t_receive;

  static constexpr size_t PACKET_SIZE = 48;
  static constexpr size_t PAYLOAD_SIZE =
      sizeof(sequence_number) + sizeof(t_send) + sizeof(t_receive);

  std::vector<uint8_t> Serialize() const {
    std::vector<uint8_t> buffer(PACKET_SIZE, 0);
    size_t offset = 0;

    std::memcpy(buffer.data() + offset, &sequence_number,
                sizeof(sequence_number));
    offset += sizeof(sequence_number);

    uint64_t net_t_send = htobe64(t_send);
    std::memcpy(buffer.data() + offset, &net_t_send, sizeof(net_t_send));
    offset += sizeof(t_send);

    uint64_t net_t_receive = htobe64(t_receive);
    std::memcpy(buffer.data() + offset, &net_t_receive, sizeof(net_t_receive));

    return buffer;
  }

  static SyncProbe Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < PAYLOAD_SIZE) {  // 1 + 8 + 8 bytes
      throw std::runtime_error("Malformed packet received: insufficient bytes");
    }
    SyncProbe probe{};
    size_t offset = 0;

    std::memcpy(&probe.sequence_number, data.data() + offset,
                sizeof(probe.sequence_number));
    offset += sizeof(probe.sequence_number);

    uint64_t net_t_send;
    std::memcpy(&net_t_send, data.data() + offset, sizeof(net_t_send));
    probe.t_send = be64toh(net_t_send);
    offset += sizeof(net_t_send);

    uint64_t net_t_receive;
    std::memcpy(&net_t_receive, data.data() + offset, sizeof(net_t_receive));
    probe.t_receive = be64toh(net_t_receive);
    return probe;
  }
};