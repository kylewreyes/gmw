#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

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
  std::shared_ptr<NetworkDriver> network_driver = std::make_shared<NetworkDriverImpl>();
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
  std::vector<std::shared_future<InitialShare_Message>> initial_futures;
  std::vector<std::shared_future<Nop_Message>> nop_futures;

  for (int i = 0; i < input.size(); i++)
  {
    InitialWireInput wire_initial_input = input[i];

    int wire_owner = wire_initial_input.party_index;

    if (wire_owner == my_party)
    {
      std::vector<int> shares = sd.generate_shares(wire_initial_input.value);

      for (int j = 0; j < num_parties; j++)
      {
        int curr_share = shares[j];

        if (j == my_party)
        {
          shares[i] = curr_share;
        }
        else
        {
          peer_links[i].DispatchShare(curr_share);
        }
      }
    }
    else
    {
      shares[i] = peer_links[i].ReceiveShare();
    }
  }

  std::cout << "party " << my_party << "in a system with " << num_parties << "parties and input of size" << input.size() << "\n";

  for (int i = 0; i < input.size(); i++)
  {
    std::cout << "party " << my_party << "got share " << i << "to be " << shares[i] << "\n";
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
      int ot_accumulator = 0;

      for (int i = 0; i < num_parties; i++)
      {
        if (my_party == i)
        {
          continue;
        }

        int is_OT_sender = my_party < i;

        int ot_response;
        if (is_OT_sender)
        {
          int bit = generate_bit();
          ot_response = bit;

          std::vector<int> choices = {bit, bit ^ left, bit ^ right, bit ^ left ^ right};

          // Need to async send these to other person
        }
        else
        {
          // 0, 0 -> 0
          // 1, 0 -> 1
          // 0, 1 -> 2
          // 1, 1 -> 3
          int choice_bit = left + (2 * right);

          // TODO(neil): CHANGE ME!
          ot_response = 1;
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

  return 0;
}