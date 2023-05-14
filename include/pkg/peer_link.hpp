#pragma once

#include "../../include-shared/circuit.hpp"
#include "../../include/drivers/cli_driver.hpp"
#include "../../include/drivers/crypto_driver.hpp"
#include "../../include/drivers/network_driver.hpp"
#include "../../include/drivers/ot_driver.hpp"

class PeerLink
{
public:
  PeerLink(
      int my_party,
      int other_party, std::string address, int port,
      std::shared_ptr<NetworkDriver> network_driver,
      std::shared_ptr<CryptoDriver> crypto_driver);

  void HandleKeyExchange();

  // Sharing
  void DispatchShare(int share);
  int ReceiveShare();

  // OT (circuit evaluation)
  int HandleAndGate();

private:
  int my_party;
  int other_party;

  std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock> SendFirstHandleKeyExchange();
  std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock> ReadFirstHandleKeyExchange();

  std::shared_ptr<NetworkDriver> network_driver;
  std::shared_ptr<CryptoDriver> crypto_driver;

  CryptoPP::SecByteBlock AES_key;
  CryptoPP::SecByteBlock HMAC_key;

  std::shared_ptr<OTDriver> ot_driver;
};
