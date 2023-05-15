#pragma once

#include "../../include-shared/circuit.hpp"
#include "../../include/drivers/cli_driver.hpp"
#include "../../include/drivers/crypto_driver.hpp"
#include "../../include/drivers/network_driver.hpp"
#include "../../include/drivers/ot_driver.hpp"

class PeerLink
{
public:
  PeerLink(std::shared_ptr<NetworkDriver> network_driver, std::shared_ptr<CryptoDriver> crypto_driver);
  bool is_send_first = 0;

  std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock> SendFirstHandleKeyExchange();
  std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock> ReadFirstHandleKeyExchange();

  std::shared_ptr<boost::asio::ip::tcp::socket> socket;

  CryptoPP::SecByteBlock AES_key;
  CryptoPP::SecByteBlock HMAC_key;

private:
  std::shared_ptr<NetworkDriver> network_driver;
  std::shared_ptr<CryptoDriver> crypto_driver;
};
