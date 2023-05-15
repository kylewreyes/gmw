#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <future>
#include <random>
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
        << "Usage: ./participant <addr file> <circuit file> <input file> <my party>"
        << std::endl;
    return 1;
  }
  std::string addr_file = argv[1];
  std::string circuit_file = argv[2];
  std::string input_file = argv[3];
  int my_party = std::stoi(argv[4]);

  Circuit circuit = parse_circuit(circuit_file);

  std::vector<InitialWireInput> input = parse_input(input_file);
  std::vector<std::string> addrs = parse_addrs(addr_file);
  int num_parties = addrs.size();
  int my_port = std::stoi(string_split(addrs[my_party], ':')[1]);

  // ===============================
  // PREPARE TO CONNECT TO PEERS
  // ===============================
  std::shared_ptr<NetworkDriverImpl> network_driver = std::make_shared<NetworkDriverImpl>();
  std::shared_ptr<CryptoDriver> crypto_driver = std::make_shared<CryptoDriver>();

  // ==========================
  // ESTABLISH SOCKETS
  // ==========================

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(0, 250);

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
    std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
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
    auto &pl = peer_links.at(i);

    std::cout << "Doing read-first key exchange with party " << i << std::endl;
    pl.ReadFirstHandleKeyExchange();
    std::cout << "AES_key for party " << i << " is " << byteblock_to_string(pl.AES_key) << std::endl;
  }

  for (int i = my_party + 1; i < num_parties; i++)
  {
    auto &pl = peer_links.at(i);

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
  for (int i = 0; i < input.size(); i++)
  {
    InitialWireInput wire_initial_input = input[i];

    int wire_owner = wire_initial_input.party_index;
    int wire_value = wire_initial_input.value;

    if (wire_owner == my_party)
    {
      std::cout << "OWNER: wire " << i << std::endl;

      std::vector<int> wire_shares = sd.generate_shares(wire_initial_input.value);
      for (int j = 0; j < num_parties; j++)
      {
        int curr_share = wire_shares[j];

        if (j == my_party)
        {
          shares[i] = curr_share;
        }
        else
        {
          std::cout << "Sending secret share of " << curr_share << " to party " << j << std::endl;
          auto &pl = peer_links.at(j);
          pl.SendSecretShare(curr_share);
        }
      }
    }
    else
    {
      auto &pl = peer_links.at(wire_owner);
      auto my_share = pl.ReceiveSecretShare();

      std::cout << "Received secret share " << my_share << " for wire " << i << " from party " << wire_owner << std::endl;
      shares[i] = my_share;
    }
  }

  // =====================
  // GMW Circuit evaluation
  // ======================
  for (Gate g : circuit.gates)
  {
    int left = shares[g.lhs];
    int right = shares[g.rhs];

    if (g.type == GateType::XOR_GATE)
    {
      shares[g.output] = (left ^ right);
    }
    else if (g.type == GateType::AND_GATE)
    {
      std::cout << "Executing AND gate" << std::endl;

      int ot_accumulator = 0;

      for (int i = 0; i < num_parties; i++)
      {
        if (my_party == i)
        {
          continue;
        }
        auto &pl = peer_links.at(i);

        int ot_response;
        if (my_party < i)
        {
          int bit = generate_bit();
          ot_response = bit;

          std::vector<int> choices = {bit, bit ^ left, bit ^ right, bit ^ left ^ right};
          std::cout << "For party " << i << "acting as OT sender with values" << choices[0] << ", " << choices[1] << ", " << choices[2] << ", " << choices[3] << std::endl;

          pl.OT_send(choices);
        }
        else
        {
          // 0, 0 -> 0
          // 1, 0 -> 1
          // 0, 1 -> 2
          // 1, 1 -> 3
          int choice_bit = left + (2 * right);
          ot_response = pl.OT_recv(choice_bit);
          std::cout << "For party " << i << "acting os OT receiver with choice bit " << choice_bit << ". Got response: " << ot_response << std::endl;
        }

        ot_accumulator += ot_response;
      }

      ot_accumulator += (left * right);
      ot_accumulator = ot_accumulator % 2;

      shares[g.output] = ot_accumulator;
    }
    else if (g.type == GateType::NOT_GATE)
    {
      if (my_party == 0)
      {
        shares[g.output] = 1 - left;
      }
      else
      {
        shares[g.output] = left;
      }
    }
    else
    {
      throw std::runtime_error("Invalid gate type found");
    }
  }

  // ==================================
  // OUTPUT RECONSTRUCTION FROM SHARES
  // ==================================
  std::string output_share = "";
  for (int i = circuit.output_length; i > 0; i--)
  {
    auto curr_share = shares.at(i);
    output_share += std::to_string(curr_share);
  }

  std::cout << "Final output share bit-string: " << output_share << std::endl;

  return 0;
}