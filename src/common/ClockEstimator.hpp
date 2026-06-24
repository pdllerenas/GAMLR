#include <algorithm>
#include <array>
#include <boost/math/distributions/gamma.hpp>
#include <boost/math/statistics/linear_regression.hpp>
#include <boost/math/statistics/univariate_statistics.hpp>
#include <chrono>
#include <cmath>
#include <numeric>
#include <span>
#include <stdexcept>

#include "../../data/quantiles_data.hpp"
#include "Socket.hpp"
#include "SyncProbe.hpp"
#include "Types.hpp"

using namespace Quantiles;

class ClockEstimator {
 private:
  UDPClient& client;

  double ComputeQuantile(GammaParameters params,
                         std::span<const double> z_grid) {
    size_t nx = rho_bin.size();
    // size_t ny = beta_bin.size();

    auto it_x = std::lower_bound(rho_bin.begin(), rho_bin.end(), params.rho);
    auto it_y = std::lower_bound(beta_bin.begin(), beta_bin.end(), params.beta);

    if (it_x == rho_bin.end() || it_x == rho_bin.begin() ||
        it_y == beta_bin.end() || it_y == beta_bin.begin()) {
      throw std::out_of_range(
          "Requested point is outside the interpolation grid.");
    }

    size_t ix = std::distance(rho_bin.begin(), it_x) - 1;
    size_t iy = std::distance(beta_bin.begin(), it_y) - 1;

    double tx = (params.rho - rho_bin[ix]) / (rho_bin[ix + 1] - rho_bin[ix]);
    double ty =
        (params.beta - beta_bin[iy]) / (beta_bin[iy + 1] - beta_bin[iy]);

    double z00 = z_grid[iy * nx + ix];
    double z10 = z_grid[iy * nx + (ix + 1)];
    double z01 = z_grid[(iy + 1) * nx + ix];
    double z11 = z_grid[(iy + 1) * nx + (ix + 1)];

    double bottom_interp = std::lerp(z00, z10, tx);
    double top_interp = std::lerp(z01, z11, tx);

    return std::lerp(bottom_interp, top_interp, ty);
  }

  std::array<double, NUM_PACKETS> ComputeQuantiles(GammaParameters params,
                                                   bool special_case) {
    std::array<std::span<const double>, NUM_PACKETS> quantiles;
    std::array<double, NUM_PACKETS> theoretical_quantiles;
    if (special_case) {
      quantiles = {quantil40_bin, quantil45_bin, quantil50_bin, quantil55_bin,
                   quantil60_bin};
    } else {
      quantiles = {quantil16_bin, quantil33_bin, quantil50_bin, quantil67_bin,
                   quantil83_bin};
    }
    for (size_t i = 0; i < quantiles.size(); ++i) {
      theoretical_quantiles[i] = ComputeQuantile(params, quantiles[i]);
    }
    return theoretical_quantiles;
  }

  GammaParameters CalculateGammaParameters(Stats stats) {
    double rho = 4.0 / std::pow(stats.skewness, 2.0);
    double beta = (stats.stddev * stats.skewness) / 2.0;
    double shift = stats.mean - ((2 * stats.stddev) / stats.skewness);
    return {rho, beta, shift, stats.mean};
  }

  double FitShiftedGamma(const std::vector<double>& forward_transit_times,
                         GammaParameters params, bool special_case) {
    std::array<double, NUM_PACKETS> theoretical_quantiles =
        ComputeQuantiles(params, special_case);
    std::array<double, NUM_PACKETS> ftt_doubles;
    for (size_t i = 0; i < NUM_PACKETS; ++i) {
      ftt_doubles[i] = forward_transit_times[i];
    }
    auto [a, b] = boost::math::statistics::simple_ordinary_least_squares(
        ftt_doubles, theoretical_quantiles);
    return a / b;
  }

  uint64_t GetCurrentTime() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  Stats CalculateStats(const std::vector<double>& forward_transit_times) {
    auto [mean, variance] = boost::math::statistics::mean_and_sample_variance(
        forward_transit_times);
    double stddev = std::sqrt(variance);
    double skewness = boost::math::statistics::skewness(forward_transit_times);

    return {mean, variance, stddev, skewness, 0};
  }

 public:
  explicit ClockEstimator(UDPClient& network_client) : client(network_client) {}
  double CalculateOffset() {
    std::vector<double> forward_transit_times;
    std::vector<double> packet_separation;

    for (uint8_t i = 0; i < NUM_PACKETS; ++i) {
      SyncProbe probe{i, GetCurrentTime(), 0};
      client.Send(probe.Serialize());
    }

    SyncProbe previous_probe{};

    for (uint8_t i = 0; i < NUM_PACKETS; ++i) {
      std::vector<uint8_t> reply = client.Receive(1024);
      SyncProbe replied_probe = SyncProbe::Deserialize(reply);

      int64_t t_rx = static_cast<int64_t>(replied_probe.t_receive);
      int64_t t_tx = static_cast<int64_t>(replied_probe.t_send);

      forward_transit_times.push_back(static_cast<double>(t_rx - t_tx) /
                                      1000.0);
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

    double ips_tolerance_us =
        std::chrono::duration<double, std::milli>(IPS_TOLERANCE).count();
    double lower_bound_us =
        std::chrono::duration<double, std::milli>(M_SEC_LOWERBOUND).count();
    bool is_low_variance(stats.stddev < lower_bound_us);
    bool is_ideal_average = (stats.packet_separation_avg >= 0.0) &&
                            (stats.packet_separation_avg < ips_tolerance_us);

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