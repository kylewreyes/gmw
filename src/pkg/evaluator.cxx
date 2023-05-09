#include "../../include/pkg/evaluator.hpp"
#include "../../include-shared/constants.hpp"
#include "../../include-shared/util.hpp"
#include "../../include-shared/logger.hpp"

/*
Syntax to use logger:
  CUSTOM_LOG(lg, debug) << "your message"
See logger.hpp for more modes besides 'debug'
*/
namespace
{
  src::severity_logger<logging::trivial::severity_level> lg;
}

/**
 * Constructor. Note that the OT_driver is left uninitialized.
 */
EvaluatorClient::EvaluatorClient(Circuit circuit,
                                 std::shared_ptr<NetworkDriver> network_driver,
                                 std::shared_ptr<CryptoDriver> crypto_driver)
{
  this->circuit = circuit;
  this->network_driver = network_driver;
  this->crypto_driver = crypto_driver;
  this->cli_driver = std::make_shared<CLIDriver>();
}

/**
 * Handle key exchange with evaluator
 */
std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock>
EvaluatorClient::HandleKeyExchange()
{
  // Generate private/public DH keys
  auto dh_values = this->crypto_driver->DH_initialize();

  // Listen for g^b
  std::vector<unsigned char> garbler_public_value_data = network_driver->read();
  DHPublicValue_Message garbler_public_value_s;
  garbler_public_value_s.deserialize(garbler_public_value_data);

  // Send g^a
  DHPublicValue_Message evaluator_public_value_s;
  evaluator_public_value_s.public_value = std::get<2>(dh_values);
  std::vector<unsigned char> evaluator_public_value_data;
  evaluator_public_value_s.serialize(evaluator_public_value_data);
  network_driver->send(evaluator_public_value_data);

  // Recover g^ab
  CryptoPP::SecByteBlock DH_shared_key = crypto_driver->DH_generate_shared_key(
      std::get<0>(dh_values), std::get<1>(dh_values),
      garbler_public_value_s.public_value);
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
 * 1) Receive the garbled circuit and the garbler's input
 * 2) Reconstruct the garbled circuit and input the garbler's inputs
 * 3) Retrieve evaluator's inputs using OT
 * 4) Evaluate gates in order (use `GarbledCircuit::evaluate_gate` to help!)
 * 5) Send final labels to the garbler
 * 6) Receive final output
 * `input` is the evaluator's input for each gate
 * You may find `string_to_byteblock` useful for converting OT output to wires
 * Disconnect and throw errors only for invalid MACs
 */
std::string EvaluatorClient::run(std::vector<int> input)
{
  // Key exchange
  auto keys = this->HandleKeyExchange();
  std::cout << "key exchange completed successfully\n";

  auto AES_key = keys.first;
  auto HMAC_key = keys.second;

  // 1) Receive the garbled gates
  GarblerToEvaluator_GarbledTables_Message tables_data;

  auto [plain_bytes, verified] =
      this->crypto_driver->decrypt_and_verify(AES_key, HMAC_key, this->network_driver->read());
  if (!verified) {
    this->network_driver->disconnect();
    throw std::runtime_error("Invalid garbled tabled bytes");
  }
  tables_data.deserialize(plain_bytes);

  std::cout << "successfully got tables from the garbler, of length " << tables_data.garbled_tables.size() << std::endl;

  // 1) Receive the garbler inputs
  GarblerToEvaluator_GarblerInputs_Message wire_msg;

  auto [plain_bytes2, verified2] =
    this->crypto_driver->decrypt_and_verify(AES_key, HMAC_key, this->network_driver->read());
  if (!verified2) {
    this->network_driver->disconnect();
    throw std::runtime_error("Invalid garbler inputs bytes");
  }
  wire_msg.deserialize(plain_bytes2);
  auto garbled_wires = wire_msg.garbler_inputs;

  // 2) Reconstruct the garbled circuit and input the garbler's inputs
  GarbledCircuit gc;
  gc.garbled_gates = tables_data.garbled_tables;
  gc.garbled_wires = garbled_wires;

  // 3) Retrieve evaluator inputs using OT
  for (int choice_bit: input) {
    CryptoPP::SecByteBlock payload = string_to_byteblock(this->ot_driver->OT_recv(choice_bit));
    GarbledWire wire;
    wire.value = payload;
    gc.garbled_wires.push_back(wire);
  }

  // Clean grow for the output wires
  for (int i = 0; i < this->circuit.output_length; i++) {
    GarbledWire output_wire;
    gc.garbled_wires.push_back(output_wire);
  }

  std::cout << "ot is over, garbled wires length is " << gc.garbled_wires.size() << std::endl;
  std::cout << "garbled gates length is " << gc.garbled_gates.size() << std::endl;

  // 4) Evaluate the gates in order
  for (int i = 0; i < this->circuit.gates.size(); i++) {
    Gate g = this->circuit.gates.at(i);

    auto lhs = gc.garbled_wires.at(g.lhs);
    auto rhs = gc.garbled_wires.at(g.rhs);

    auto wire = this->evaluate_gate(gc.garbled_gates.at(i), lhs, rhs);
    gc.garbled_wires.at(g.output) = wire;
  }

  // 5) Send final labels to the garbler
  EvaluatorToGarbler_FinalLabels_Message final_labels;
  // MAYBE: we just grab the output number of wires from the garbled wires vector?
  std::cout << "garbled wires length is " << garbled_wires.size() << " and output length is " << this->circuit.output_length << std::endl;

  std::vector<GarbledWire> final_wires(garbled_wires.end() - this->circuit.output_length, garbled_wires.end());
  final_labels.final_labels = final_wires;

  this->network_driver->send(this->crypto_driver->encrypt_and_tag(AES_key, HMAC_key, &final_labels));

  std::cout << "going to consider the final output" << std::endl;

  // 6) Receive the final output
  GarblerToEvaluator_FinalOutput_Message final_output;
  auto [final_bytes, verified3] = this->crypto_driver->decrypt_and_verify(AES_key, HMAC_key, this->network_driver->read());

  if (!verified3) {
    this->network_driver->disconnect();
    throw std::runtime_error("Unable to verify final message");
  }
  final_output.deserialize(final_bytes);

  return final_output.final_output;
}

/**
 * Evaluate gate.
 * You may find CryptoPP::xorbuf and CryptoDriver::hash_inputs useful.
 * To determine if a decryption is valid, use verify_decryption.
 * To retrieve the label from a decryption, use snip_decryption.
 */
GarbledWire EvaluatorClient::evaluate_gate(GarbledGate gate, GarbledWire lhs,
                                           GarbledWire rhs)
{
  GarbledWire gw;
  auto enc_len = LABEL_LENGTH + LABEL_TAG_LENGTH;

  std::cout << "hash inputs\n";
  auto hashed_inputs = this->crypto_driver->hash_inputs(lhs.value, rhs.value);

  for (auto ciphertext: gate.entries) {
    std::cout << "gate entry ciphertext" << byteblock_to_string(ciphertext) << std::endl;
    CryptoPP::SecByteBlock decryption(enc_len);
    CryptoPP::xorbuf(decryption, ciphertext, hashed_inputs, enc_len);

    std::cout << "going to verify decryption \n";
    if (this->verify_decryption(decryption)) {
      std::cout << "about to snip decryption\n";
      gw.value = this->snip_decryption(decryption);
      return gw;
    }
    std::cout << "not valid decypriont\n";
  }

  return gw;
}

/**
 * Verify decryption. A valid dec should end with LABEL_TAG_LENGTH bits of 0s.
 */
bool EvaluatorClient::verify_decryption(CryptoPP::SecByteBlock decryption)
{
  CryptoPP::SecByteBlock trail(decryption.data() + LABEL_LENGTH,
                               LABEL_TAG_LENGTH);
  return byteblock_to_integer(trail) == CryptoPP::Integer::Zero();
}

/**
 * Returns the first LABEL_LENGTH bits of a decryption.
 */
CryptoPP::SecByteBlock EvaluatorClient::snip_decryption(CryptoPP::SecByteBlock decryption)
{
  CryptoPP::SecByteBlock head(decryption.data(), LABEL_LENGTH);
  return head;
}
