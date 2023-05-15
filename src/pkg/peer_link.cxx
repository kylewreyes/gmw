#include "../../include/pkg/peer_link.hpp"

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
PeerLink::PeerLink(std::shared_ptr<NetworkDriver> network_driver, std::shared_ptr<CryptoDriver> crypto_driver)
{
  this->network_driver = network_driver;
  this->crypto_driver = crypto_driver;
}

std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock>
PeerLink::ReadFirstHandleKeyExchange()
{
  // Generate private/public DH keys
  auto dh_values = this->crypto_driver->DH_initialize();

  // Listen for g^b
  std::cout << "performing read-first handle key exchange\n";
  std::vector<unsigned char> garbler_public_value_data = network_driver->socket_read(socket);
  std::cout << "done with socket read" << std::endl;
  DHPublicValue_Message garbler_public_value_s;
  garbler_public_value_s.deserialize(garbler_public_value_data);

  // Send g^a
  DHPublicValue_Message evaluator_public_value_s;
  evaluator_public_value_s.public_value = std::get<2>(dh_values);
  std::vector<unsigned char> evaluator_public_value_data;
  evaluator_public_value_s.serialize(evaluator_public_value_data);
  network_driver->socket_send(socket, evaluator_public_value_data);

  // Recover g^ab
  CryptoPP::SecByteBlock DH_shared_key = crypto_driver->DH_generate_shared_key(
      std::get<0>(dh_values), std::get<1>(dh_values),
      garbler_public_value_s.public_value);
  CryptoPP::SecByteBlock AES_key =
      this->crypto_driver->AES_generate_key(DH_shared_key);
  CryptoPP::SecByteBlock HMAC_key =
      this->crypto_driver->HMAC_generate_key(DH_shared_key);
  std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock> keys = std::make_pair(AES_key, HMAC_key);
  return keys;
}

/**
 * Handle key exchange with evaluator
 */
std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock>
PeerLink::SendFirstHandleKeyExchange()
{
  // Generate private/public DH keys
  auto dh_values = this->crypto_driver->DH_initialize();

  // Send g^b
  DHPublicValue_Message garbler_public_value_s;
  garbler_public_value_s.public_value = std::get<2>(dh_values);
  std::vector<unsigned char> garbler_public_value_data;
  garbler_public_value_s.serialize(garbler_public_value_data);
  network_driver->socket_send(socket, garbler_public_value_data);
  std::cout << "sent!" << std::endl;

  // Listen for g^a
  std::vector<unsigned char> evaluator_public_value_data =
      network_driver->socket_read(socket);
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
  return keys;
}