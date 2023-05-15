#include "../../include/pkg/peer_link.hpp"

#include "../../include-shared/constants.hpp"
#include "../../include-shared/util.hpp"
#include "../../include-shared/messages.hpp"
#include "../../include-shared/logger.hpp"

auto &mod_exp = CryptoPP::ModularExponentiation;
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
PeerLink::PeerLink(std::shared_ptr<boost::asio::ip::tcp::socket> socket, std::shared_ptr<NetworkDriver> network_driver, std::shared_ptr<CryptoDriver> crypto_driver)
{
  this->socket = socket;
  this->network_driver = network_driver;
  this->crypto_driver = crypto_driver;
}

void PeerLink::GossipSend(std::string bit_string)
{
  FinalGossip_Message msg;
  msg.bit_string = bit_string;

  auto bytes = this->crypto_driver->encrypt_and_tag(AES_key, HMAC_key, &msg);
  this->network_driver->socket_send(socket, bytes);
}

std::string PeerLink::GossipReceive()
{
  FinalGossip_Message msg;

  auto bytes = this->network_driver->socket_read(socket);
  auto [data, verified] = this->crypto_driver->decrypt_and_verify(AES_key, HMAC_key, bytes);
  if (!verified)
  {
    throw std::runtime_error("error verifying final gossip message");
  }

  msg.deserialize(data);
  return msg.bit_string;
}

/*
 * Send one of m[0], ..., m[n - 1] using OT, where n = m.size(). This function
 * should:
 * 1) Sample a public DH value and send it to the receiver
 * 2) Receive the receiver's public value
 * 3) Encrypt m[0], ..., m[n - 1] using different keys
 * 4) Send the encrypted values
 * You may find `byteblock_to_integer` and `integer_to_byteblock` useful
 * Disconnect and throw errors only for invalid MACs
 */
void PeerLink::OT_send(std::vector<int> m)
{
  // Implement me!
  auto &mod_inv = CryptoPP::EuclideanMultiplicativeInverse;
  auto [dh_obj, dh_priv_key, dh_pub_key] = crypto_driver->DH_initialize();

  // 1) Sample a public DH value and send it to the receiver
  SenderToReceiver_OTPublicValue_Message sender_pub_key_msg;
  sender_pub_key_msg.public_value = dh_pub_key;
  std::vector<unsigned char> bytes =
      crypto_driver->encrypt_and_tag(AES_key, HMAC_key, &sender_pub_key_msg);
  network_driver->socket_send(socket, bytes);

  // 2) Receive the receiver's public value
  bytes = network_driver->socket_read(socket);
  auto [plain_bytes, verified] =
      crypto_driver->decrypt_and_verify(AES_key, HMAC_key, bytes);
  if (!verified)
  {
    throw std::runtime_error(
        "OT_send: Received invalid HMAC for receiver's public value");
  }
  ReceiverToSender_OTPublicValue_Message receiver_pub_key_msg;
  receiver_pub_key_msg.deserialize(plain_bytes);

  // 3) Encrypt m[0], ..., m[n - 1] using different keys
  CryptoPP::Integer B = byteblock_to_integer(receiver_pub_key_msg.public_value);
  CryptoPP::Integer A = byteblock_to_integer(dh_pub_key);
  CryptoPP::Integer a = byteblock_to_integer(dh_priv_key);

  SenderToReceiver_OTEncryptedValues_Message ot_msg;
  for (int i = 0; i < m.size(); i++)
  {
    // HKDF input: (B / A^i)^a
    CryptoPP::Integer k_i = (B * mod_inv(mod_exp(A, i, DL_P), DL_P)) % DL_P;

    auto k_to_hash = crypto_driver->DH_generate_shared_key(
        dh_obj, dh_priv_key, integer_to_byteblock(k_i));
    SecByteBlock k = crypto_driver->AES_generate_key(k_to_hash);
    auto [e, iv] = crypto_driver->AES_encrypt(k, std::to_string(m[i]));

    ot_msg.encryptions.push_back(e);
    ot_msg.ivs.push_back(iv);
  }

  // 4) Send the encrypted values
  bytes = crypto_driver->encrypt_and_tag(AES_key, HMAC_key, &ot_msg);
  network_driver->socket_send(socket, bytes);
}

/*
 * Receive m_c using OT. This function should:
 * 1) Read the sender's public value
 * 2) Respond with our public value that depends on our choice bit
 * 3) Generate the appropriate key and decrypt the appropriate ciphertext
 * You may find `byteblock_to_integer` and `integer_to_byteblock` useful
 * Disconnect and throw errors only for invalid MACs
 */
int PeerLink::OT_recv(int choice_bit)
{
  // Implement me!
  // 1) Read the sender's public value
  std::vector<unsigned char> bytes = network_driver->socket_read(socket);
  auto [plain_bytes, verified] =
      crypto_driver->decrypt_and_verify(AES_key, HMAC_key, bytes);
  if (!verified)
  {
    throw std::runtime_error(
        "OT_recv: Received invalid HMAC for sender's public value");
  }
  SenderToReceiver_OTPublicValue_Message sender_pub_key_msg;
  sender_pub_key_msg.deserialize(plain_bytes);
  CryptoPP::Integer A = byteblock_to_integer(sender_pub_key_msg.public_value);

  // 2) Respond with our public value that depends on our choice bit
  auto [dh_obj, dh_priv_key, dh_pub_key] = crypto_driver->DH_initialize();

  ReceiverToSender_OTPublicValue_Message receiver_pub_key_msg;
  CryptoPP::Integer B = byteblock_to_integer(dh_pub_key);
  B = (B * mod_exp(A, choice_bit, DL_P)) % DL_P;
  dh_pub_key = integer_to_byteblock(B);
  receiver_pub_key_msg.public_value = dh_pub_key;
  bytes =
      crypto_driver->encrypt_and_tag(AES_key, HMAC_key, &receiver_pub_key_msg);
  network_driver->socket_send(socket, bytes);

  // 3) Generate the appropriate key and decrypt the appropriate ciphertext
  bytes = network_driver->socket_read(socket);
  auto [plain_bytes_2, verified_2] =
      crypto_driver->decrypt_and_verify(AES_key, HMAC_key, bytes);
  if (!verified)
  {
    throw std::runtime_error(
        "OT_recv: Received invalid HMAC for sender's final message");
  }

  SenderToReceiver_OTEncryptedValues_Message ot_msg;
  ot_msg.deserialize(plain_bytes_2);
  auto kc_to_hash = crypto_driver->DH_generate_shared_key(
      dh_obj, dh_priv_key, sender_pub_key_msg.public_value);
  SecByteBlock kc = crypto_driver->AES_generate_key(kc_to_hash);

  auto str = crypto_driver->AES_decrypt(kc, ot_msg.ivs[choice_bit],
                                        ot_msg.encryptions[choice_bit]);

  return std::stoi(str);
}

void PeerLink::SendSecretShare(int share)
{
  InitialShare_Message msg;
  msg.share_value = share;

  std::vector<unsigned char> bytes = this->crypto_driver->encrypt_and_tag(this->AES_key, this->HMAC_key, &msg);
  this->network_driver->socket_send(this->socket, bytes);
}

int PeerLink::ReceiveSecretShare()
{
  auto bytes = this->network_driver->socket_read(socket);
  auto [plain_bytes, verified] = crypto_driver->decrypt_and_verify(AES_key, HMAC_key, bytes);
  if (!verified)
  {
    throw std::runtime_error("Error verifying secret share reception message");
  }

  InitialShare_Message msg;
  msg.deserialize(plain_bytes);

  return msg.share_value;
}

void PeerLink::ReadFirstHandleKeyExchange()
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

  this->AES_key = AES_key;
  this->HMAC_key = HMAC_key;
}

/**
 * Handle key exchange with evaluator
 */
void PeerLink::SendFirstHandleKeyExchange()
{
  // Generate private/public DH keys
  auto dh_values = this->crypto_driver->DH_initialize();

  // Send g^b
  DHPublicValue_Message garbler_public_value_s;
  garbler_public_value_s.public_value = std::get<2>(dh_values);
  std::vector<unsigned char> garbler_public_value_data;
  garbler_public_value_s.serialize(garbler_public_value_data);
  network_driver->socket_send(socket, garbler_public_value_data);

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

  this->AES_key = AES_key;
  this->HMAC_key = HMAC_key;
}