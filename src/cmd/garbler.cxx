#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include "../../include-shared/circuit.hpp"
#include "../../include-shared/logger.hpp"
#include "../../include-shared/util.hpp"
#include "../../include/pkg/garbler.hpp"
#include "../../include/drivers/peer_driver.hpp"

/*
 * Usage: ./yaos_garbler <circuit file> <input file> <address> <port>
 */
int main(int argc, char *argv[])
{
  // Initialize logger
  initLogger();

  // Parse args
  if (!(argc == 5))
  {
    std::cout
        << "Usage: ./yaos_garbler <circuit file> <input file> <address> <port>"
        << std::endl;
    return 1;
  }
  std::string circuit_file = argv[1];
  std::string input_file = argv[2];
  std::string address = argv[3];
  int port = atoi(argv[4]);

  // Parse circuit.
  Circuit circuit = parse_circuit(circuit_file);

  // Connect to peers
  std::vector<std::string> vec = {"foo", "bar"};
  PeerDriver pd(0, vec);

  // Do the initial secret sharing
  std::vector<int> wires;
  for (int i = 0; i < wires; i++)
  {
    // If we own the wire, generate the secret shares and send to everyone
    // Otherwise,
  }

  // Parse input.
  std::vector<int> input = parse_input(input_file);

  // Connect to network driver.
  std::shared_ptr<NetworkDriver> network_driver =
      std::make_shared<NetworkDriverImpl>();
  network_driver->listen(port);
  std::shared_ptr<CryptoDriver> crypto_driver =
      std::make_shared<CryptoDriver>();

  // Create garbler then run.
  GarblerClient garbler = GarblerClient(circuit, network_driver, crypto_driver);
  garbler.run(input);
  return 0;
}
