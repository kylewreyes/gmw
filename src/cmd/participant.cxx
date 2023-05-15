#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <future>
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::seconds

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
  int my_port = std::stoi(string_split(addrs[my_party], ':')[1]);

  // ===============================
  // PREPARE TO CONNECT TO PEERS
  // ===============================
  std::unordered_map<std::string, int> addr_mapping;
  for (int i = 0; i < addrs.size(); i++)
  {
    addr_mapping[addrs[i]] = i;
  }

  std::shared_ptr<NetworkDriverImpl> network_driver = std::make_shared<NetworkDriverImpl>();
  std::shared_ptr<CryptoDriver> crypto_driver = std::make_shared<CryptoDriver>();

  // We only need to clean this up at the end
  std::thread listen_thread([network_driver, my_party, my_port]()
                            {
        std::cout << "Party " << my_party << " starting to listen on port " << my_port << " for " << my_party << " connections\n";
        network_driver->listen(my_party, my_port); });

  // ==========================
  // ESTABLISH CONNECTIONS
  // ==========================
  for (int i = my_party + 1; i < num_parties; i++)
  {
    auto addr_parts = parse_addr(addrs[i]);

    // Need to make the connection to peer i
    network_driver->connect(i, addr_parts.first, addr_parts.second);

    std::cout << "Party " << my_party << " successfully connected to peer " << i << "\n";
  }

  std::this_thread::sleep_for(std::chrono::seconds(5));

  // ===========================
  // KEY EXCHANGE
  // ===========================

  // For all the send_conns, in a thread do a SendFirstHandleKeyExchange
  // For all the recv_conns, in a thread to a ReceiveFirstHandleKeyExchange

  std::vector<std::thread> threads;

  for (int i = my_party + 1; i < num_parties; i++)
  {
    PeerLink pl(network_driver, crypto_driver);

    threads.push_back(std::thread([&pl, i]()
                                  { pl.SendFirstHandleKeyExchange(i); }));
  }

  for (auto s : network_driver->recv_conns)
  {
    PeerLink pl(network_driver, crypto_driver);

    threads.push_back(std::thread([&pl, s]()
                                  { pl.ReadFirstHandleKeyExchange(s); }));
  }

  for (auto &thr : threads)
  {
    thr.join();
  }

  // For the things less than us: listen in a thread, and do key exchange in that thread
  // For the things greater than us, send in a thread and do key exchange in that thread

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

  // Clean up the listener thread
  listen_thread.detach();

  return 0;
}