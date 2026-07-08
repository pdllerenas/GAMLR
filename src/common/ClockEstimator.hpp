#include <algorithm>
#include <array>
#include <boost/math/distributions/gamma.hpp>
#include <boost/math/statistics/linear_regression.hpp>
#include <boost/math/statistics/univariate_statistics.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "Socket.hpp"
#include "SyncProbe.hpp"
#include "Types.hpp"

/**
 * @brief Estimates clock offset using one-way delay measurements and a shifted
 *        gamma distribution model.
 *
 * ClockEstimator sends a sequence of SyncProbe packets over the provided
 * network link, collects timing measurements, and derives a clock offset
 * estimate using either a minimum observed delay or a gamma distribution fit.
 */
class ClockEstimator {
private:
  INetworkLink
      &link; /**< Reference to the network transport used for probe exchange. */
  size_t packet_size;

  /**
   * @brief Compute an array of theoretical quantiles for gamma fitting.
   *
   * @param params Gamma distribution parameters derived from sample stats.
   * @param special_case If true, use an alternate set of quantile bands for
   *                     low-variance conditions.
   * @return Array of NUM_PACKETS quantile values.
   */
  std::array<double, NUM_PACKETS> ComputeQuantiles(GammaParameters params,
                                                   bool special_case) {
    std::array<double, NUM_PACKETS> theoretical_quantiles;
    std::array<double, NUM_PACKETS> probabilities;
    if (special_case) {
      probabilities = {0.40, 0.45, 0.50, 0.55, 0.60};
    } else {
      probabilities = {0.166666, 0.333333, 0.50, 0.666666, 0.833333};
    }
    boost::math::gamma_distribution<double> dist(params.rho, params.beta);
    for (size_t i = 0; i < NUM_PACKETS; ++i) {
      theoretical_quantiles[i] = boost::math::quantile(dist, probabilities[i]);
    }
    return theoretical_quantiles;
  }

  /**
   * @brief Calculate gamma distribution parameters from measured statistics.
   *
   * @param stats Observed mean, variance, stddev, and skewness.
   * @return GammaParameters containing rho, beta, shift, and mean.
   */
  GammaParameters CalculateGammaParameters(Stats stats) {
    double rho = 4.0 / std::pow(stats.skewness, 2.0);
    double beta = (stats.stddev * stats.skewness) / 2.0;
    double shift = stats.mean - ((2 * stats.stddev) / stats.skewness);

    if (beta > 15 || beta < 0) {
      throw std::out_of_range("beta out of limits.");
    }
    if (rho > 3000 || rho < 0) {
      throw std::out_of_range("rho out of limits.");
    }
    return {rho, beta, shift, stats.mean};
  }

  /**
   * @brief Fit a shifted gamma distribution to the measured transit times.
   *
   * @param forward_transit_times Forward transit delays in milliseconds.
   * @param params Gamma distribution parameters.
   * @param special_case Use the alternate low-variance quantile set if true.
   * @return Estimated gamma coefficient from ordinary least squares.
   *
   * @throws std::runtime_error If the regression slope is too small.
   */
  double FitShiftedGamma(const std::vector<double> &forward_transit_times,
                         GammaParameters params, bool special_case) {
    std::array<double, NUM_PACKETS> theoretical_quantiles =
        ComputeQuantiles(params, special_case);
    std::array<double, NUM_PACKETS> ftt_doubles;
    for (size_t i = 0; i < NUM_PACKETS; ++i) {
      ftt_doubles[i] = forward_transit_times[i];
    }
    // Returns the values that best fit a + b * theoretical_quantiles =
    // ftt_doubles
    const auto [a, b] = boost::math::statistics::simple_ordinary_least_squares(
        theoretical_quantiles, ftt_doubles);
    if (std::abs(b) < 1e-12)
      throw std::runtime_error("slope too small");
    return a;
  }

  /**
   * @brief Return the current system time in microseconds since epoch.
   *
   * @return Current time in microseconds.
   */
  uint64_t GetCurrentTime() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::high_resolution_clock::now().time_since_epoch())
        .count();
  }

  /**
   * @brief Compute summary statistics for forward transit times.
   *
   * @param forward_transit_times Observed forward transit delays in ms.
   * @return Stats containing mean, variance, stddev, and skewness.
   *
   * @throws std::runtime_error If skewness is too close to zero.
   */
  Stats CalculateStats(const std::vector<double> &forward_transit_times) {
    auto [mean, variance] = boost::math::statistics::mean_and_sample_variance(
        forward_transit_times);
    double stddev = std::sqrt(variance);
    double skewness = boost::math::statistics::skewness(forward_transit_times);
    if (std::abs(skewness) < 1e-12)
      throw std::runtime_error("Skewness too small");
    return {mean, variance, stddev, skewness, 0};
  }

public:
  /**
   * @brief Construct the clock estimator with a network link reference.
   *
   * @param network_link The network transport used for sending probes.
   */
  explicit ClockEstimator(INetworkLink &network_link, size_t pkt_size = 48)
      : link(network_link), packet_size(pkt_size) {}

  /**
   * @brief Estimate the clock offset using measured one-way delays.
   *
   * Sends NUM_PACKETS SyncProbe packets, collects replies, computes statistics,
   * and then chooses either a minimum observed delay or a gamma fit estimate.
   *
   * @return Estimated offset in milliseconds.
   */
  double CalculateOffset() {
    int max_retries = 5;
    while (max_retries-- > 0) {
      try {
        std::vector<double> forward_transit_times;
        std::vector<double> packet_separation;

        for (uint8_t i = 0; i < NUM_PACKETS; ++i) {
          SyncProbe probe{i, GetCurrentTime(), 0};
          link.Send(probe.Serialize(packet_size));

          if (i < NUM_PACKETS - 1) {
            std::this_thread::sleep_for(INTER_PACKET_SEPARATION);
          }
        }

        SyncProbe previous_probe{};

        for (uint8_t i = 0; i < NUM_PACKETS; ++i) {
          std::vector<uint8_t> reply = link.Receive(65536);
          SyncProbe replied_probe = SyncProbe::Deserialize(reply);

          if (replied_probe.sequence_number != i) {
            throw std::runtime_error(
                "Packet sequence mismatch (out of order or stale)");
          }

          int64_t t_rx = static_cast<int64_t>(replied_probe.t_receive);
          int64_t t_tx = static_cast<int64_t>(replied_probe.t_send);

          forward_transit_times.push_back(static_cast<double>(t_rx - t_tx) /
                                          1000.0);
          std::cout << "OWD[" << static_cast<int>(i)
                    << "] = " << forward_transit_times[i] << '\n';
          if (i > 0) {
            int64_t prev_rx = static_cast<int64_t>(previous_probe.t_receive);
            packet_separation.push_back(static_cast<double>(t_rx - prev_rx) /
                                        1000.0);
          }
          previous_probe = replied_probe;
        }

        double avg_packet_separation =
            boost::math::statistics::mean(packet_separation);

        Stats stats = CalculateStats(forward_transit_times);
        stats.packet_separation_avg = avg_packet_separation;

        double gamma_coefficient;

        double ips_tolerance_ms =
            std::chrono::duration<double, std::milli>(IPS_TOLERANCE).count();
        double lower_bound_ms =
            std::chrono::duration<double, std::milli>(M_SEC_LOWERBOUND).count();
        bool is_low_variance(stats.stddev < lower_bound_ms);
        bool is_ideal_average =
            (stats.packet_separation_avg >= 0.0) &&
            (stats.packet_separation_avg < ips_tolerance_ms);

        GammaParameters params = CalculateGammaParameters(stats);
        std::cout << "rho: " << params.rho << "\nbeta: " << params.beta << '\n';
        if (is_low_variance && is_ideal_average) {
          gamma_coefficient = *std::min_element(forward_transit_times.begin(),
                                                forward_transit_times.end());
        } else {
          std::sort(forward_transit_times.begin(), forward_transit_times.end());
          gamma_coefficient =
              FitShiftedGamma(forward_transit_times, params, is_low_variance);
        }

        return gamma_coefficient;
      } catch (const std::system_error &e) {
        if (e.code().value() == EAGAIN ||
            e.code().value() ==
                EWOULDBLOCK) { // only catch packet lost or timeout
          std::cerr << "[Network] Packet lost. Retrying (" << max_retries
                    << " remaining).\n";
          continue;
        }
        throw; // throw if different error
      } catch (const std::out_of_range &e) {
        std::cerr << "[Network] Out of range error: " << e.what()
                  << ". Retrying (" << max_retries << " remaining).\n";
        continue;
      }
    }
    throw std::runtime_error("Failed to estimate offset: packets were lost.");
  }
};
