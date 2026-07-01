#pragma once

#include <chrono>

/**
 * @brief Aggregated statistics computed from observed forward transit delays.
 *
 * These values are used to derive the shifted gamma distribution parameters
 * and to decide whether a direct minimum delay estimate or a full fit is used.
 */
struct Stats {
  double mean;
  double variance;
  double stddev;
  double skewness;
  double packet_separation_avg;
};

/**
 * @brief Parameters for the shifted gamma distribution model.
 *
 * rho and beta control the shape of the gamma distribution, while shift
 * represents the estimated distribution offset and mean preserves the sample
 * mean.
 */
struct GammaParameters {
  double rho;
  double beta;
  double shift;
  double mean;
};

/**
 * @brief Delay between successive probe packets when sending measurement traffic.
 */
constexpr std::chrono::microseconds INTER_PACKET_SEPARATION(30000);  // 30 ms

/**
 * @brief Tolerance used to determine whether packet separation is within ideal timing.
 */
constexpr std::chrono::microseconds IPS_TOLERANCE = (INTER_PACKET_SEPARATION * 11) / 10;

/**
 * @brief Lower bound for standard deviation below which the traffic is considered low variance.
 */
constexpr std::chrono::microseconds M_SEC_LOWERBOUND(10000); // 10 ms

/**
 * @brief Number of packets exchanged during the clock offset estimation run.
 */
constexpr size_t NUM_PACKETS = 5;