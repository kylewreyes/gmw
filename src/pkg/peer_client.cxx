#include "../../include/pkg/peer_client.hpp"
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
PeerClient::PeerClient(
    int my_party,
    std::shared_ptr<CryptoDriver> crypto_driver)
{
  this->my_party = my_party;
  this->crypto_driver = crypto_driver;
}

std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock>
PeerClient::HandleKeyExchange(int other_party)
{
  bool send_first = this->my_party < other_party;

  std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock> keys;
  if (send_first)
  {
    keys = this->SendFirstHandleKeyExchange();
  }
  else
  {
    keys = this->ReadFirstHandleKeyExchange();
  }

  this->AES_key = keys.first;
  this->HMAC_key = keys.second;
}

std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock>
PeerClient::ReadFirstHandleKeyExchange()
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
 * Handle key exchange with evaluator
 */
std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock>
PeerClient::SendFirstHandleKeyExchange()
{
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