#pragma once

#include "../../include-shared/circuit.hpp"
#include "../../include/drivers/cli_driver.hpp"
#include "../../include/drivers/crypto_driver.hpp"
#include "../../include/drivers/network_driver.hpp"
#include "../../include/drivers/ot_driver.hpp"

class PeerLink
{
public:
  PeerLink(std::shared_ptr<boost::asio::ip::tcp::socket> sock, std::shared_ptr<NetworkDriver> network_driver, std::shared_ptr<CryptoDriver> crypto_driver);

  void SendFirstHandleKeyExchange();
  void ReadFirstHandleKeyExchange();

  CryptoPP::SecByteBlock AES_key;
  CryptoPP::SecByteBlock HMAC_key;

private:
  std::shared_ptr<boost::asio::ip::tcp::socket> socket;

  std::shared_ptr<NetworkDriver> network_driver;
  std::shared_ptr<CryptoDriver> crypto_driver;
};
