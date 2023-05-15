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

  // Key exchange
  void SendFirstHandleKeyExchange();
  void ReadFirstHandleKeyExchange();

  // Initial secret sharing
  void SendSecretShare(int share);
  int ReceiveSecretShare();

  // OT
  void OT_send(std::vector<int> choices);
  int OT_recv(int choice_bit);

  // Final gossip
  void GossipSend(std::string bit_string);
  std::string GossipReceive();

  CryptoPP::SecByteBlock AES_key;
  CryptoPP::SecByteBlock HMAC_key;

private:
  std::shared_ptr<boost::asio::ip::tcp::socket> socket;

  std::shared_ptr<NetworkDriver> network_driver;
  std::shared_ptr<CryptoDriver> crypto_driver;
};
