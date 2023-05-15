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
  // TODO: DO WE STILL NEED THIS?
  std::unordered_map<std::string, int> addr_mapping;
  for (int i = 0; i < addrs.size(); i++)
  {
    addr_mapping[addrs[i]] = i;
  }

  std::shared_ptr<NetworkDriverImpl> network_driver = std::make_shared<NetworkDriverImpl>();
  std::shared_ptr<CryptoDriver> crypto_driver = std::make_shared<CryptoDriver>();

  // We only need to clean this up at the end
  // TODO: Might need a mutex for sockets coz more than 1 thread is accessing it at the same time
  // std::thread listen_thread([network_driver, my_party, my_port]()
  //                           {
  //       std::cout << "Party " << my_party << " starting to listen on port " << my_port << " for " << my_party << " connections" << std::endl;
  //       network_driver->listen(my_party, my_port); });

  // ==========================
  // ESTABLISH SOCKETS
  // ==========================

  // Map from party index to their socket
  std::unordered_map<int, std::shared_ptr<boost::asio::ip::tcp::socket>> sockets;

  // Do my_party number of listens
  for (int i = 0; i < my_party; i++)
  {
    std::cout << "Listening for party " << i << std::endl;
    auto socket = network_driver->listen(my_port);
    sockets[i] = socket;
  }

  // Then the rest should be connections
  for (int i = my_party + 1; i < num_parties; i++)
  {
    std::cout << "Connecting to party " << i << std::endl;
    auto addr_parts = parse_addr(addrs[i]);
    auto socket = network_driver->connect(i, addr_parts.first, addr_parts.second);

    // It seems that if everybody calls connect() to a party at the same time, then that party
    // can't respond to everyone, and things fail. This seems to fix it... :(
    std::this_thread::sleep_for(std::chrono::seconds(1));
    sockets[i] = socket;
  }

  // ===========================
  // SETUP PEER LINKS
  // ===========================

  std::unordered_map<int, PeerLink> peer_links;
  for (auto &[other_party, socket] : sockets)
  {
    auto pl = PeerLink(socket, network_driver, crypto_driver);
    peer_links.emplace(other_party, pl);
  }

  // ==============================
  // KEY EXCHANGE
  // ==============================

  // Same trick. my_party number of listens, followed by the rest being sends
  for (int i = 0; i < my_party; i++)
  {
    auto pl = peer_links.at(i);

    std::cout << "Doing read-first key exchange with party " << i << std::endl;
    pl.ReadFirstHandleKeyExchange();
    std::cout << "AES_key for party " << i << " is " << byteblock_to_string(pl.AES_key) << std::endl;
  }

  for (int i = my_party + 1; i < num_parties; i++)
  {
    auto pl = peer_links.at(i);

    std::cout << "Doing send-first key exchange with party " << i << std::endl;
    pl.SendFirstHandleKeyExchange();
    std::cout << "AES_key for party " << i << " is " << byteblock_to_string(pl.AES_key) << std::endl;
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