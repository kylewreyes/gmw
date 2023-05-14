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
  int num_parties = addrs.size();

  // Create a peer driver
  std::vector<std::string> addrs;
  PeerDriver pd(my_party, addrs);

  // Create a ShareDriver
  ShareDriver sd(my_party, num_parties);

  // Store the shares for every single wire in the circuit
  // The first input.size() wires are populated using the initial sharing protocol below
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
      // Create and broadcast shares to everyone
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
          InitialShare_Message msg;
          msg.share_value = curr_share;
          msg.wire_number = i;

          auto f = pd.start_in_thread<InitialShare_Message, Nop_Message>(j, SEND_AE, msg);
          nop_futures.push_back(f);
        }
      }
    }
    else
    {
      Nop_Message msg;
      auto f = pd.start_in_thread<Nop_Message, InitialShare_Message>(wire_owner, SEND_AE, msg);
      initial_futures.push_back(f);
    }
  }

  // Wait for all the futures to complete
  for (auto f : initial_futures)
  {
    auto msg = f.get();
    shares[msg.wire_number] = msg.share_value;
  }

  // Don't care about these values that we get back
  for (auto f : nop_futures)
  {
    f.wait();
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
    if (g.type == GateType::XOR_GATE)
    {
      shares[g.output] = (shares[g.lhs] ^ shares[g.rhs]);
    }
    else if (g.type == GateType::AND_GATE)
    {
      // This is a bit more involved
    }
    else if (g.type == GateType::NOT_GATE)
    {
      // This should be an easy case, but it's not in the paper
    }
    else
    {
      throw std::runtime_error("Invalid gate type found")
    }
  }

  return 0;
}