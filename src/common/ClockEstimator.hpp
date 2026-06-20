#include <algorithm>
#include <array>
#include <boost/math/distributions/gamma.hpp>
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
    size_t nx = rho_bin.size() / sizeof(double);
    size_t ny = beta_bin.size() / sizeof(double);

    auto it_x = std::lower_bound(rho_bin, rho_bin + nx, params.rho);
    auto it_y = std::lower_bound(beta_bin, beta_bin + ny, params.beta);

    if (it_x == rho_bin + nx || it_x == rho_bin || it_y == beta_bin + ny ||
        it_y == beta_bin) {
      throw std::out_of_range(
          "Requested point is outside the interpolation grid.");
    }

    size_t ix = std::distance(rho_bin, it_x) - 1;
    size_t iy = std::distance(beta_bin, it_y) - 1;

    double tx = (params.rho - rho_bin[ix]) / (rho_bin[ix + 1] - rho_bin[ix]);
    double ty =
        (params.beta - beta_bin[iy]) / (beta_bin[iy + 1] - beta_bin[iy]);

    double z00 = z_grid[iy * nx + ix];
    double z10 = z_grid[iy * nx + (ix + 1)];
    double z01 = z_grid[(iy + 1) * nx + ix];
    double z11 = z_grid[(iy + 1) * nx + (ix + 1)];

    double bottom_interp = std::lerp(z00, z10, tx);
    double top_interp = std::lerp(z10, z11, tx);

    return std::lerp(bottom_interp, top_interp, ty);
  }

  std::array<double, NUM_PACKETS> ComputeQuantiles(GammaParameters params,
                                                   bool special_case) {
    std::array<std::span<uint8_t>, 5> quantiles;
    if (special_case) {
      quantiles = {quantil40_bin, quantil45_bin, quantil50_bin, quantil55_bin,
                   quantil60_bin};
    } else {
      quantiles = {quantil16_bin, quantil33_bin, quantil50_bin, quantil66_bin,
                   quantil83_bin};
    }
    for (auto& quantile : quantiles) {
      ComputeQuantile(params, quantile);
    }
    return {};
  }

  GammaParameters CalculateGammaParameters(Stats stats) {
    double rho = 4.0 / std::pow(stats.skewness, 2.0);
    double beta = (stats.stddev * stats.skewness) / 2.0;
    double shift = mean - ((2 * stats.stddev) / stats.skewness);
    return {rho, beta, shift, stats.mean};
  }

  double FitShiftedGamma(Stats stats, GammaParameters params) {
    boost::math::gamma_distribution();

    return 0.0;
  }

  uint64_t GetCurrentTime() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  Stats CalculateStats(const std::vector<uint64_t>& forward_transit_times) {
    auto [mean, variance] = boost::math::statistics::mean_and_sample_variance(
        forward_transit_times);
    double stddev = std::sqrt(variance);
    double skewness = boost::math::statistics::skewness(forward_transit_times);

    std::vector<uint64_t> packet_separation;
    for (int i = 1; i < NUM_PACKETS; ++i) {
      packet_separation.push_back(forward_transit_times[i].t_receive -
                                  forward_transit_times[i - 1].t_receive);
    }

    double avg_packet_separation =
        boost::math::statistics::mean(packet_separation);

    return {mean, variance, stddev, skewness, avg_packet_separation};
  }

 public:
  explicit ClockEstimator(UDPClient& network_client) : client(network_client) {}
  double CalculateOffset() {
    std::vector<uint64_t> forward_transit_times;

    for (uint8_t i = 0; i < NUM_PACKETS; ++i) {
      SyncProbe probe{i, GetCurrentTime(), 0};
      client.Send(probe.Serialize());
    }

    for (uint8_t i = 0; i < NUM_PACKETS; ++i) {
      std::vector<uint8_t> reply = client.Receive(1024);
      SyncProbe replied_probe = SyncProbe::Deserialize(reply);

      forward_transit_times.push_back(replied_probe.t_receive -
                                      replied_probe.t_send);
    }

    Stats stats = CalculateStats(forward_transit_times);
    GammaParameters params = CalculateGammaParameters(stats);
    double gamma_coefficient;

    double ips_tolerance_ms =
        std::chrono::duration<double, std::milli>(IPS_TOLERANCE).count();
    bool is_low_variance(stats.stddev < M_SEC_LOWERBOUND);
    bool is_ideal_average = (stats.packet_separation_avg > 0.0) &&
                            (stats.packet_separation_avg < ips_tolerance_ms);

    if (is_low_variance && is_ideal_average) {
      gamma_coefficient = std::min(forward_transit_times);
    } else {
      ComputeQuantiles()
    }

    // pending logic
    double final_offset = 0.0;
    return final_offset;
  }
};