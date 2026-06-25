#include <algorithm>
#include <array>
#include <boost/math/distributions/gamma.hpp>
#include <boost/math/statistics/linear_regression.hpp>
#include <boost/math/statistics/univariate_statistics.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <span>
#include <stdexcept>
#include <thread>

#include "../../data/quantiles_data.hpp"
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
  INetworkLink& link; /**< Reference to the network transport used for probe exchange. */

  /**
   * @brief Perform bilinear interpolation of a quantile over a rho-beta grid.
   *
   * @param params Gamma distribution parameters containing rho and beta.
   * @param z_grid Flattened quantile grid values ordered by beta rows and rho
   *               columns.
   * @return Interpolated quantile value for the requested rho/beta location.
   *
   * @throws std::out_of_range If the requested rho or beta value lies outside
   *         the available bin grid.
   */
  double ComputeQuantile(GammaParameters params,
                         std::span<const double> z_grid) {
    size_t nx = Quantiles::rho_bin.size();
    // size_t ny = beta_bin.size();

    double clamped_rho = std::clamp(params.rho, Quantiles::rho_bin.front(),
                                    Quantiles::rho_bin.back());
    double clamped_beta = std::clamp(params.beta, Quantiles::beta_bin.front(),
                                     Quantiles::beta_bin.back());

    auto it_x = std::lower_bound(Quantiles::rho_bin.begin(),
                                 Quantiles::rho_bin.end(), clamped_rho);
    auto it_y = std::lower_bound(Quantiles::beta_bin.begin(),
                                 Quantiles::beta_bin.end(), clamped_beta);

    if (it_x == Quantiles::rho_bin.end() ||
        it_x == Quantiles::rho_bin.begin() ||
        it_y == Quantiles::beta_bin.end() ||
        it_y == Quantiles::beta_bin.begin()) {
      throw std::out_of_range(
          "Requested point is outside the interpolation grid.");
    }

    size_t ix = std::distance(Quantiles::rho_bin.begin(), it_x) - 1;
    size_t iy = std::distance(Quantiles::beta_bin.begin(), it_y) - 1;

    double tx = (clamped_rho - Quantiles::rho_bin[ix]) /
                (Quantiles::rho_bin[ix + 1] - Quantiles::rho_bin[ix]);
    double ty =
        (clamped_beta - Quantiles::beta_bin[iy]) / (Quantiles::beta_bin[iy + 1] - Quantiles::beta_bin[iy]);

    double z00 = z_grid[iy * nx + ix];
    double z10 = z_grid[iy * nx + (ix + 1)];
    double z01 = z_grid[(iy + 1) * nx + ix];
    double z11 = z_grid[(iy + 1) * nx + (ix + 1)];

    double bottom_interp = std::lerp(z00, z10, tx);
    double top_interp = std::lerp(z01, z11, tx);

    return std::lerp(bottom_interp, top_interp, ty);
  }

  /**
   * @brief Compute an array of theoretical quantiles for gamma fitting.
   *
   * @param params Gamma distribution parameters derived from sample stats.
   * @param special_case If true, use an alternate set of quantile bands for
   *                     low-variance conditions.
   * @return Array of NUM_PACKETS interpolated quantile values.
   */
  std::array<double, NUM_PACKETS> ComputeQuantiles(GammaParameters params,
                                                   bool special_case) {
    std::array<std::span<const double>, NUM_PACKETS> quantiles;
    std::array<double, NUM_PACKETS> theoretical_quantiles;
    if (special_case) {
      quantiles = {Quantiles::quantil40_bin, Quantiles::quantil45_bin,
                   Quantiles::quantil50_bin, Quantiles::quantil55_bin,
                   Quantiles::quantil60_bin};
    } else {
      quantiles = {Quantiles::quantil16_bin, Quantiles::quantil33_bin,
                   Quantiles::quantil50_bin, Quantiles::quantil67_bin,
                   Quantiles::quantil83_bin};
    }
    for (size_t i = 0; i < quantiles.size(); ++i) {
      theoretical_quantiles[i] = ComputeQuantile(params, quantiles[i]);
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

    // if (beta > 15 || beta < 0) {
    //   throw std::out_of_range("beta out of limits.");
    // }
    // if (rho > 3000 || rho < 0) {
    //   throw std::out_of_range("rho out of limits.");
    // }
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
  double FitShiftedGamma(const std::vector<double>& forward_transit_times,
                         GammaParameters params, bool special_case) {
    std::array<double, NUM_PACKETS> theoretical_quantiles =
        ComputeQuantiles(params, special_case);
    std::array<double, NUM_PACKETS> ftt_doubles;
    for (size_t i = 0; i < NUM_PACKETS; ++i) {
      ftt_doubles[i] = forward_transit_times[i];
    }
    const auto [a, b] = boost::math::statistics::simple_ordinary_least_squares(
        ftt_doubles, theoretical_quantiles);
    if (std::abs(b) < 1e-12) throw std::runtime_error("slope too small");
    return a / b;
  }

  /**
   * @brief Return the current system time in microseconds since epoch.
   *
   * @return Current time in microseconds.
   */
  uint64_t GetCurrentTime() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch())
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
  Stats CalculateStats(const std::vector<double>& forward_transit_times) {
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
  explicit ClockEstimator(INetworkLink& network_link) : link(network_link) {}

  /**
   * @brief Estimate the clock offset using measured one-way delays.
   *
   * Sends NUM_PACKETS SyncProbe packets, collects replies, computes statistics,
   * and then chooses either a minimum observed delay or a gamma fit estimate.
   *
   * @return Estimated offset in milliseconds.
   */
  double CalculateOffset() {
    std::vector<double> forward_transit_times;
    std::vector<double> packet_separation;

    for (uint8_t i = 0; i < NUM_PACKETS; ++i) {
      SyncProbe probe{i, GetCurrentTime(), 0};
      link.Send(probe.Serialize());

      if (i < NUM_PACKETS - 1) {
        std::this_thread::sleep_for(INTER_PACKET_SEPARATION);
      }
    }

    SyncProbe previous_probe{};

    for (uint8_t i = 0; i < NUM_PACKETS; ++i) {
      std::vector<uint8_t> reply = link.Receive(1024);
      SyncProbe replied_probe = SyncProbe::Deserialize(reply);

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
    bool is_ideal_average = (stats.packet_separation_avg >= 0.0) &&
                            (stats.packet_separation_avg < ips_tolerance_ms);

    if (is_low_variance && is_ideal_average) {
      gamma_coefficient = *std::min_element(forward_transit_times.begin(),
                                            forward_transit_times.end());
    } else {
      GammaParameters params = CalculateGammaParameters(stats);
      std::sort(forward_transit_times.begin(), forward_transit_times.end());
      gamma_coefficient =
          FitShiftedGamma(forward_transit_times, params, is_low_variance);
    }

    return gamma_coefficient;
  }
};