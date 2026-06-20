#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>

struct SyncProbe {
  uint8_t sequence_number;
  uint64_t t_send;
  uint64_t t_receive;

  std::vector<uint8_t> Serialize() const {
    std::vector<uint8_t> buffer(sizeof(sequence_number) + sizeof(t_send) +
                                sizeof(t_receive));
    size_t offset = 0;

    std::memcpy(buffer.data() + offset, &sequence_number,
                sizeof(sequence_number));
    offset += sizeof(sequence_number);

    std::memcpy(buffer.data() + offset, &t_send, sizeof(t_send));
    offset += sizeof(t_send);

    std::memcpy(buffer.data() + offset, &t_receive, sizeof(t_receive));
    return buffer;
  }

  static SyncProbe Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 17) {  // 1 + 8 + 8 bytes
      throw std::runtime_error("Malformed packet received: insufficient bytes");
    }
    SyncProbe probe{};
    size_t offset = 0;

    std::memcpy(&probe.sequence_number, data.data() + offset,
                sizeof(probe.sequence_number));
    offset += sizeof(probe.sequence_number);

    std::memcpy(&probe.t_send, data.data() + offset, sizeof(probe.t_send));
    offset += sizeof(probe.t_send);

    std::memcpy(&probe.t_receive, data.data() + offset,
                sizeof(probe.t_receive));
    return probe;
  }
};