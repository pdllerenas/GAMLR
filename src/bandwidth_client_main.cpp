#include <iostream>
#include <boost/random/mersenne_twister.hpp>

#include "common/Socket.hpp"
#include "common/BandwidthEstimator.hpp"


int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: ./avbw_client <Server_IP> <Port>\n";
    return EXIT_FAILURE;
  }

  std::string ip = argv[1];
  uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

  UDPClient client(ip, port);
	BandwidthEstimator bwe(client);

	boost::random::mt19937 gen(42);

	double bw = bwe.CalculateBandwidth(gen);

}
