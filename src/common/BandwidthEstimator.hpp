#pragma once

#include <boost/random/exponential_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <chrono>
#include <iostream>
#include <thread>

#include "Socket.hpp"
#include "SyncProbe.hpp"

class BandwidthEstimator {
private:
  INetworkLink &link;

  uint64_t GetCurrentTime() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  void SpinSleep(std::chrono::microseconds duration) {
    auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::high_resolution_clock::now() - start < duration) {
      // Spin lock to prevent the OS scheduler from stealing the thread
    }
  }

public:
  explicit BandwidthEstimator(INetworkLink &_link) : link(_link) {}
  double CalculateBandwidth(boost::random::mt19937 &rng) {
    constexpr uint32_t PACKET_SIZE = 1400;
    constexpr uint32_t CAPACITY_MBPS = 50;
    constexpr uint8_t NUM_PACKET_PAIRS =
        10; // number of pairs (so 20 packets in total)
    static_assert(NUM_PACKET_PAIRS * 2 < 255);

    constexpr uint64_t D_IN_US = (PACKET_SIZE * 8) / CAPACITY_MBPS;
    constexpr std::chrono::microseconds D_IN(D_IN_US);
    constexpr double D_IN_DOUBLE = static_cast<double>(D_IN_US);

    // Measures time gap for packet pairs
    std::vector<double> packet_separation;
    packet_separation.reserve(NUM_PACKET_PAIRS);

    for (uint8_t i = 0; i < 2 * NUM_PACKET_PAIRS; i += 2) {
      SyncProbe probe_a{i, GetCurrentTime(), 0};
      link.Send(probe_a.Serialize(PACKET_SIZE));

      SpinSleep(D_IN);

      SyncProbe probe_b{static_cast<uint8_t>(i + 1), GetCurrentTime(), 0};
      link.Send(probe_b.Serialize(PACKET_SIZE));

      boost::random::exponential_distribution<double> exp_dist(
          1.0 / (D_IN_DOUBLE * 100.0));

      std::chrono::duration<double, std::micro> t(exp_dist(rng));
      std::cout << "Sleeping for " << t.count() << " us\n";

      std::this_thread::sleep_for(t);
    }

    return 0.0;
  }
};
