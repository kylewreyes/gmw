

/*


A participant needs a:


- circuit file
- who owns what file
- input file (with who owns what)
- address file

- go through every single gate

*/

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include "../../include-shared/circuit.hpp"
#include "../../include-shared/logger.hpp"
#include "../../include-shared/util.hpp"
#include "../../include/pkg/garbler.hpp"
#include "../../include/drivers/peer_driver.hpp"
#include "../../include/drivers/share_driver.hpp"

/*
 * Usage: ./participant <circuit file> <input file> <address> <port>
 */
int main(int argc, char *argv[])
{
  // Initialize logger
  initLogger();

  // Parse args
  if (!(argc == 5))
  {
    std::cout
        << "Usage: ./participant <my party> <addr file> <circuit file> <input file>"
        << std::endl;
    return 1;
  }
  int my_party = std::stoi(argv[1]);
  std::string addr_file = argv[2];
  std::string circuit_file = argv[3];
  std::string input_file = argv[4];

  // TODO: parse the circuit better
  Circuit circuit = parse_circuit(circuit_file);

  std::vector<InitialWireInput> input = parse_input(input_file);
  std::vector<std::string> addrs = parse_addrs(addr_file);

  // Create a peer driver
  std::vector<std::string> addrs;
  int my_party = 2;
  PeerDriver pd(my_party, addrs);

  // Create a ShareDriver
  ShareDriver sd(my_party, addr_file.size());

  // TODO: Need to figure out how long to make this. Probably # of wires
  std::vector<int> shares(circuit.num_wire, 0);

  // Do the initial wire sharing
  int num_initial_wires = 10;
  for (int i = 0; i < num_initial_wires; i++)
  {
    InitialWireInput wire_initial_input = input.at(i);

    // Generate secret shares and send them out
    if (wire_initial_input.party_index == my_party)
    {
      // TODO: broadcast these to everyone
      std::vector<int> shares = sd.generate_shares(wire_initial_input.value);
    }
    else
    {
      // TODO: Receive the secret share for the person who owns it
    }
  }

  // Run the computation
  for (int i = 0; i < circuit.gates.size(); i++)
  {
    Gate g = circuit.gates[i];

    // TODO: Switch on the type and decide what to do
  }

  return 0;
}
