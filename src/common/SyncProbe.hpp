#pragma once

#include <endian.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

/**
 * @brief Represents a synchronization probe packet used for clock offset measurement.
 *
 * The packet contains a sequence number and two timestamp fields, one for when
 * the probe was sent and one for when it was received. Serialization uses
 * network byte order so packets can be safely transmitted across the network.
 */
struct SyncProbe {
  uint8_t sequence_number;
  uint64_t t_send;
  uint64_t t_receive;

  static constexpr size_t PAYLOAD_SIZE =
      sizeof(sequence_number) + sizeof(t_send) + sizeof(t_receive);

  /**
   * @brief Serialize the SyncProbe into a fixed-size network packet.
   *
   * @return A 48-byte buffer containing the serialized probe.
   */
  std::vector<uint8_t> Serialize(size_t target_size = 48) const {
    if (target_size < PAYLOAD_SIZE) target_size = PAYLOAD_SIZE;
    std::vector<uint8_t> buffer(target_size, 0);
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

  /**
   * @brief Deserialize a SyncProbe packet from a byte buffer.
   *
   * @param data Byte buffer containing the serialized probe.
   * @return Deserialized SyncProbe instance.
   *
   * @throws std::runtime_error If the provided buffer is too small.
   */
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
