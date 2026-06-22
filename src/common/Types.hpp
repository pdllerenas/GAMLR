#pragma once

#include <chrono>

struct Stats {
  double mean;
  double variance;
  double stddev;
  double skewness;
  double packet_separation_avg;
};

struct GammaParameters {
  double rho;
  double beta;
  double shift;
  double mean;
};

constexpr std::chrono::microseconds INTER_PACKET_SEPARATION(30000);  // 30 ms
constexpr std::chrono::microseconds IPS_TOLERANCE = (INTER_PACKET_SEPARATION * 11) / 10;
constexpr std::chrono::microseconds M_SEC_LOWERBOUND(500);

constexpr size_t NUM_PACKETS = 5;