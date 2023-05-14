#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>

#include "../../include-shared/circuit.hpp"
#include "../../include-shared/logger.hpp"
#include "../../include-shared/util.hpp"
#include "../../include/pkg/peer_link.hpp"
#include "../../include/drivers/share_driver.hpp"

/*
 * Usage: ./participant <circuit file> <input file> <address> <port>
 */
int main(int argc, char *argv[])
{
  // Initialize logger
  initLogger();

  // ======================
  // INPUT PARSING
  // ======================
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

  Circuit circuit = parse_circuit(circuit_file);

  std::vector<InitialWireInput> input = parse_input(input_file);
  std::vector<std::string> addrs = parse_addrs(addr_file);
  int num_parties = addrs.size();

  // ==========================
  // CONNECT TO PEERS
  // ==========================
  std::unordered_map<std::string, int> addr_mapping;
  for (int i = 0; i < addrs.size(); i++)
  {
    addr_mapping[addrs[i]] = i;
  }

  std::shared_ptr<NetworkDriver> network_driver = std::make_shared<NetworkDriverImpl>(addr_mapping);
  std::shared_ptr<CryptoDriver> crypto_driver = std::make_shared<CryptoDriver>();

  PeerLink pl(0, 0, "", 0, network_driver, crypto_driver);
  std::vector<PeerLink> peer_links(num_parties, pl);

  for (int i = 0; i < num_parties; i++)
  {
    if (i == my_party)
    {
      continue;
    }

    peer_links[i] = PeerLink(my_party, i, addrs[i], 0, network_driver, crypto_driver);
  }

  // ===========================
  // KEY EXCHANGE
  // ===========================
  for (int i = 0; i < num_parties; i++)
  {
    if (i == my_party)
    {
      continue;
    }

    peer_links[i].HandleKeyExchange();
  }

  // ===========================
  // SECRET SHARES
  // ===========================
  ShareDriver sd(my_party, num_parties);
  std::vector<int> shares(circuit.num_wire, 0);

  // ========================
  // Initial wire sharing
  // ========================

  // =====================
  // GMW Circuit evaluation
  // ======================

  return 0;
}