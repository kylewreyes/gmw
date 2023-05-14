#pragma once

#include "../../include-shared/circuit.hpp"
#include "../../include/drivers/cli_driver.hpp"
#include "../../include/drivers/crypto_driver.hpp"
#include "../../include/drivers/network_driver.hpp"
#include "../../include/drivers/ot_driver.hpp"

class PeerClient
{
public:
  PeerClient(int my_party, std::shared_ptr<CryptoDriver> crypto_driver);

  std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock> HandleKeyExchange(int my_party);

private:
  int my_party;

  std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock> SendFirstHandleKeyExchange();
  std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock> ReadFirstHandleKeyExchange();

  std::shared_ptr<NetworkDriver> network_driver;
  std::shared_ptr<CryptoDriver> crypto_driver;

  CryptoPP::SecByteBlock AES_key;
  CryptoPP::SecByteBlock HMAC_key;

  // We'll get to this in a moment.
  // std::shared_ptr<OTDriver> ot_driver;
  // std::shared_ptr<CLIDriver> cli_driver;
};
