#include <boost/math/distributions/gamma.hpp>
#include <chrono>

#include "Socket.hpp"
#include "SyncProbe.hpp"

class ClockEstimator {
 private:
  UDPClient& client;

  double FitShiftedGamma(const std::vector<uint64_t>& transit_times) {
    return 0.0;
  }

  uint64_t GetCurrentTime() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

 public:
  explicit ClockEstimator(UDPClient& network_client) : client(network_client) {}
  double CalculateOffset() {
    std::vector<uint64_t> forward_transit_times;
    std::vector<uint64_t> packet_separation;

    for (uint8_t i = 0; i < 5; ++i) {
      SyncProbe probe{i, GetCurrentTime(), 0};
      client.Send(probe.Serialize());
    }

    SyncProbe previous_probe{};

    for (uint8_t i = 0; i < 5; ++i) {
      std::vector<uint8_t> reply = client.Receive(1024);
      SyncProbe replied_probe = SyncProbe::Deserialize(reply);

      forward_transit_times.push_back(replied_probe.t_receive -
                                      replied_probe.t_send);
      if (i > 0) {
        packet_separation.push_back(replied_probe.t_receive -
                                    previous_probe.t_receive);
      }

      previous_probe = replied_probe;
    }

    double min_forward_ott = FitShiftedGamma(forward_transit_times);

    // pending logic
    double final_offset = 0.0;
    return final_offset;
  }
};