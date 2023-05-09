#include "../../include/drivers/ot_driver.hpp"

#include <crypto++/cryptlib.h>
#include <crypto++/elgamal.h>
#include <crypto++/files.h>
#include <crypto++/hkdf.h>
#include <crypto++/nbtheory.h>
#include <crypto++/queue.h>
#include <crypto++/sha.h>

#include <iostream>
#include <stdexcept>
#include <string>

#include "../../include-shared/constants.hpp"
#include "../../include-shared/messages.hpp"
#include "../../include-shared/util.hpp"
#include "crypto++/base64.h"
#include "crypto++/dsa.h"
#include "crypto++/osrng.h"
#include "crypto++/rsa.h"

auto& mod_exp = CryptoPP::ModularExponentiation;

/*
 * Constructor
 */
OTDriver::OTDriver(
    std::shared_ptr<NetworkDriver> network_driver,
    std::shared_ptr<CryptoDriver> crypto_driver,
    std::pair<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock> keys) {
  this->network_driver = network_driver;
  this->crypto_driver = crypto_driver;
  this->AES_key = keys.first;
  this->HMAC_key = keys.second;
  this->cli_driver = std::make_shared<CLIDriver>();
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
void OTDriver::OT_send(std::vector<std::string> m) {
  // Implement me!
  auto& mod_inv = CryptoPP::EuclideanMultiplicativeInverse;
  auto [dh_obj, dh_priv_key, dh_pub_key] = crypto_driver->DH_initialize();

  // 1) Sample a public DH value and send it to the receiver
  SenderToReceiver_OTPublicValue_Message sender_pub_key_msg;
  sender_pub_key_msg.public_value = dh_pub_key;
  std::vector<unsigned char> bytes =
      crypto_driver->encrypt_and_tag(AES_key, HMAC_key, &sender_pub_key_msg);
  network_driver->send(bytes);

  // 2) Receive the receiver's public value
  bytes = network_driver->read();
  auto [plain_bytes, verified] =
      crypto_driver->decrypt_and_verify(AES_key, HMAC_key, bytes);
  if (!verified) {
    network_driver->disconnect();
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
  for (int i = 0; i < m.size(); i++) {
    // HKDF input: (B / A^i)^a
    CryptoPP::Integer k_i = (B * mod_inv(mod_exp(A, i, DL_P), DL_P)) % DL_P;

    auto k_to_hash = crypto_driver->DH_generate_shared_key(
        dh_obj, dh_priv_key, integer_to_byteblock(k_i));
    SecByteBlock k = crypto_driver->AES_generate_key(k_to_hash);
    auto [e, iv] = crypto_driver->AES_encrypt(k, m[i]);

    ot_msg.encryptions.push_back(e);
    ot_msg.ivs.push_back(iv);
  }

  // 4) Send the encrypted values
  bytes = crypto_driver->encrypt_and_tag(AES_key, HMAC_key, &ot_msg);
  network_driver->send(bytes);
}

/*
 * Receive m_c using OT. This function should:
 * 1) Read the sender's public value
 * 2) Respond with our public value that depends on our choice bit
 * 3) Generate the appropriate key and decrypt the appropriate ciphertext
 * You may find `byteblock_to_integer` and `integer_to_byteblock` useful
 * Disconnect and throw errors only for invalid MACs
 */
std::string OTDriver::OT_recv(int choice_bit) {
  // Implement me!
  // 1) Read the sender's public value
  std::vector<unsigned char> bytes = network_driver->read();
  auto [plain_bytes, verified] =
      crypto_driver->decrypt_and_verify(AES_key, HMAC_key, bytes);
  if (!verified) {
    network_driver->disconnect();
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
  network_driver->send(bytes);

  // 3) Generate the appropriate key and decrypt the appropriate ciphertext
  bytes = network_driver->read();
  auto [plain_bytes_2, verified_2] =
      crypto_driver->decrypt_and_verify(AES_key, HMAC_key, bytes);
  if (!verified) {
    network_driver->disconnect();
    throw std::runtime_error(
        "OT_recv: Received invalid HMAC for sender's final message");
  }

  SenderToReceiver_OTEncryptedValues_Message ot_msg;
  ot_msg.deserialize(plain_bytes_2);
  auto kc_to_hash = crypto_driver->DH_generate_shared_key(
      dh_obj, dh_priv_key, sender_pub_key_msg.public_value);
  SecByteBlock kc = crypto_driver->AES_generate_key(kc_to_hash);

  return crypto_driver->AES_decrypt(kc, ot_msg.ivs[choice_bit],
                                    ot_msg.encryptions[choice_bit]);
}