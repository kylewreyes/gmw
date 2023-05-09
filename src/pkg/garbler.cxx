#include "../../include/pkg/garbler.hpp"

#include <crypto++/misc.h>

#include <algorithm>
#include <random>

#include "../../include-shared/constants.hpp"
#include "../../include-shared/logger.hpp"
#include "../../include-shared/util.hpp"

/*
Syntax to use logger:
  CUSTOM_LOG(lg, debug) << "your message"
See logger.hpp for more modes besides 'debug'
*/
namespace {
src::severity_logger<logging::trivial::severity_level> lg;
}

/**
 * Constructor. Note that the OT_driver is left uninitialized.
 */
GarblerClient::GarblerClient(Circuit circuit,
                             std::shared_ptr<NetworkDriver> network_driver,
                             std::shared_ptr<CryptoDriver> crypto_driver) {
  this->circuit = circuit;
  this->network_driver = network_driver;
  this->crypto_driver = crypto_driver;
  this->cli_driver = std::make_shared<CLIDriver>();
}

/**
 * Handle key exchange with evaluator
 */
std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock>
GarblerClient::HandleKeyExchange() {
  // Generate private/public DH keys
  auto dh_values = this->crypto_driver->DH_initialize();

  // Send g^b
  DHPublicValue_Message garbler_public_value_s;
  garbler_public_value_s.public_value = std::get<2>(dh_values);
  std::vector<unsigned char> garbler_public_value_data;
  garbler_public_value_s.serialize(garbler_public_value_data);
  network_driver->send(garbler_public_value_data);

  // Listen for g^a
  std::vector<unsigned char> evaluator_public_value_data =
      network_driver->read();
  DHPublicValue_Message evaluator_public_value_s;
  evaluator_public_value_s.deserialize(evaluator_public_value_data);

  // Recover g^ab
  CryptoPP::SecByteBlock DH_shared_key = crypto_driver->DH_generate_shared_key(
      std::get<0>(dh_values), std::get<1>(dh_values),
      evaluator_public_value_s.public_value);
  CryptoPP::SecByteBlock AES_key =
      this->crypto_driver->AES_generate_key(DH_shared_key);
  CryptoPP::SecByteBlock HMAC_key =
      this->crypto_driver->HMAC_generate_key(DH_shared_key);
  auto keys = std::make_pair(AES_key, HMAC_key);
  this->ot_driver =
      std::make_shared<OTDriver>(network_driver, crypto_driver, keys);
  return keys;
}

/**
 * run. This function should:
 * 1) Generate a garbled circuit from the given circuit in this->circuit
 * 2) Send the garbled circuit to the evaluator
 * 3) Send garbler's input labels in the clear
 * 4) Send evaluator's input labels using OT
 * 5) Receive final labels, recover and reveal final output
 * `input` is the evaluator's input for each gate
 * Final output should be a string containing only "0"s or "1"s
 * Throw errors only for invalid MACs
 */
std::string GarblerClient::run(std::vector<int> input) {
  // Key exchange
  auto keys = this->HandleKeyExchange();
  auto AES_key = keys.first;
  auto HMAC_key = keys.second;

  // 1) Generated a garbled circuit
  auto garbled_labels = this->generate_labels(this->circuit);
  auto garbled_gates = this->generate_gates(this->circuit, garbled_labels);

  // 2) Send the garbled circuit to the evaluator
  GarblerToEvaluator_GarbledTables_Message table;
  table.garbled_tables = garbled_gates;

  this->network_driver->send(this->crypto_driver->encrypt_and_tag(AES_key, HMAC_key, &table));

  // 3) Send the garbler's input labels in the clear
  GarblerToEvaluator_GarblerInputs_Message inputs;
  inputs.garbler_inputs = this->get_garbled_wires(garbled_labels, input, 0);

  this->network_driver->send(this->crypto_driver->encrypt_and_tag(AES_key, HMAC_key, &inputs));

  // 4) MAYBE: Send evaluator's input labels using OT
  // Do we index into the garbled_labels zero and one to get the right wire?
  for (int i = 0; i < this->circuit.evaluator_input_length; i++) {
    int offset = this->circuit.garbler_input_length + i;

    std::string m0 = byteblock_to_string(garbled_labels.zeros.at(offset).value);
    std::string m1 = byteblock_to_string(garbled_labels.ones.at(offset).value);

    this->ot_driver->OT_send(m0, m1);
  }

  // 5) Receive the final labels, recover and reveal final output
  auto [final_msg_bytes, verified] = this->crypto_driver->decrypt_and_verify(
    AES_key, HMAC_key, this->network_driver->read());
  if (!verified) {
    throw std::runtime_error("Invalid final message MAC");
  }

  EvaluatorToGarbler_FinalLabels_Message final_labels_msg;  
  final_labels_msg.deserialize(final_msg_bytes);

  auto final_labels = final_labels_msg.final_labels;

  std::string output = "";
  for (GarbledWire final_label: final_labels) {
    for (auto zero_label: garbled_labels.zeros) {
      if (zero_label.value == final_label.value) {
        output += "0";
        continue;
      }
    }

    for (auto one_label: garbled_labels.ones) {
      if (one_label.value == final_label.value) {
        output += "1";
        continue;
      }
    }

    // TODO: This shouldn't happen. What should we do?
  }

  GarblerToEvaluator_FinalOutput_Message final_output;
  final_output.final_output = output;

  this->network_driver->send(this->crypto_driver->encrypt_and_tag(AES_key, HMAC_key, &final_output));

  return output;
}

/**
 * Generate the gates for the circuit.
 * You may find `std::random_shuffle` useful
 *
 * Note: `std::random_shuffle` has been removed since C++ 17. It would work w/
 * GCC because it was kept for backwards compatibility.
 */
std::vector<GarbledGate> GarblerClient::generate_gates(Circuit circuit,
                                                       GarbledLabels labels) {
  // Implement me!
  std::vector<GarbledGate> gates;
  for (auto& g : circuit.gates) {
    GarbledGate gg;
    GarbledWire dummy_wire;
    dummy_wire.value = DUMMY_RHS;
    GarbledWire lhs_zero = labels.zeros[g.lhs];
    GarbledWire lhs_one = labels.ones[g.lhs];
    GarbledWire rhs_zero = labels.zeros[g.rhs];
    GarbledWire rhs_one = labels.ones[g.rhs];
    GarbledWire output_zero = labels.zeros[g.output];
    GarbledWire output_one = labels.ones[g.output];

    switch (g.type) {
      case GateType::AND_GATE:
        gg.entries.push_back(encrypt_label(lhs_zero, rhs_zero, output_zero));
        gg.entries.push_back(encrypt_label(lhs_zero, rhs_one, output_zero));
        gg.entries.push_back(encrypt_label(lhs_one, rhs_zero, output_zero));
        gg.entries.push_back(encrypt_label(lhs_one, rhs_one, output_one));
        break;
      case GateType::XOR_GATE:
        gg.entries.push_back(encrypt_label(lhs_zero, rhs_zero, output_zero));
        gg.entries.push_back(encrypt_label(lhs_zero, rhs_one, output_one));
        gg.entries.push_back(encrypt_label(lhs_one, rhs_zero, output_one));
        gg.entries.push_back(encrypt_label(lhs_one, rhs_one, output_zero));
        break;
      case GateType::NOT_GATE:
        gg.entries.push_back(encrypt_label(lhs_zero, dummy_wire, output_one));
        gg.entries.push_back(encrypt_label(lhs_one, dummy_wire, output_zero));
        break;
      default:
        break;
    }

    std::default_random_engine rng;
    std::shuffle(gg.entries.begin(), gg.entries.end(), rng);
    gates.push_back(gg);
  }
  return gates;
}

/**
 * Generate *all* labels for the circuit.
 * To generate an individual label, use `generate_label`.
 */
GarbledLabels GarblerClient::generate_labels(Circuit circuit) {
  // Implement me!
  GarbledLabels gl;
  for (int i = 0; i < circuit.num_wire; i++) {
    GarbledWire zero_label;
    zero_label.value = generate_label();
    gl.zeros.push_back(zero_label);

    GarbledWire one_label;
    one_label.value = generate_label();
    gl.ones.push_back(one_label);
  }
  return gl;
}

/**
 * Generate encrypted label. Tags LABEL_TAG_LENGTH trailing 0s to end before
 * encrypting. You may find CryptoPP::xorbuf and CryptoDriver::hash_inputs
 * useful.
 */
CryptoPP::SecByteBlock GarblerClient::encrypt_label(GarbledWire lhs,
                                                    GarbledWire rhs,
                                                    GarbledWire output) {
  // Implement me!
  // Resizes the block and zeroes out the new memory
  output.value.CleanGrow(LABEL_LENGTH + LABEL_TAG_LENGTH);
  CryptoPP::SecByteBlock key = crypto_driver->hash_inputs(lhs.value, rhs.value);
  // Because output was passed by value, it was copied. So, we don't have to
  // worry about modifying the original value in output.
  CryptoPP::xorbuf(output.value.BytePtr(), key.BytePtr(),
                   LABEL_LENGTH + LABEL_TAG_LENGTH);
  return output.value;
}

/**
 * Generate label.
 */
CryptoPP::SecByteBlock GarblerClient::generate_label() {
  CryptoPP::SecByteBlock label(LABEL_LENGTH);
  CryptoPP::OS_GenerateRandomBlock(false, label, label.size());
  return label;
}

/*
 * Given a set of 0/1 labels and an input vector of 0's and 1's, returns the
 * labels corresponding to the inputs starting at begin.
 */
std::vector<GarbledWire> GarblerClient::get_garbled_wires(
    GarbledLabels labels, std::vector<int> input, int begin) {
  std::vector<GarbledWire> res;
  for (int i = 0; i < input.size(); i++) {
    switch (input[i]) {
      case 0:
        res.push_back(labels.zeros[begin + i]);
        break;
      case 1:
        res.push_back(labels.ones[begin + i]);
        break;
      default:
        std::cerr << "INVALID INPUT CHARACTER" << std::endl;
    }
  }
  return res;
}
