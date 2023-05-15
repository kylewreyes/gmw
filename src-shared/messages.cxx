#include "../include-shared/messages.hpp"

#include "../include-shared/util.hpp"

// ================================================
// MESSAGE TYPES
// ================================================

/**
 * Get message type.
 */
MessageType::T get_message_type(std::vector<unsigned char> &data)
{
  return (MessageType::T)data[0];
}

// ================================================
// SERIALIZERS
// ================================================

/**
 * Puts the bool b into the end of data.
 */
int put_bool(bool b, std::vector<unsigned char> &data)
{
  data.push_back((char)b);
  return 1;
}

/**
 * Puts the string s into the end of data.
 */
int put_string(std::string s, std::vector<unsigned char> &data)
{
  // Put length
  int idx = data.size();
  data.resize(idx + sizeof(size_t));
  size_t str_size = s.size();
  std::memcpy(&data[idx], &str_size, sizeof(size_t));

  // Put string
  data.insert(data.end(), s.begin(), s.end());
  return data.size() - idx;
}

/**
 * Puts the integer i into the end of data.
 */
int put_integer(CryptoPP::Integer i, std::vector<unsigned char> &data)
{
  return put_string(CryptoPP::IntToString(i), data);
}

/**
 * Puts the nest bool from data at index idx into b.
 */
int get_bool(bool *b, std::vector<unsigned char> &data, int idx)
{
  *b = (bool)data[idx];
  return 1;
}

/**
 * Puts the nest string from data at index idx into s.
 */
int get_string(std::string *s, std::vector<unsigned char> &data, int idx)
{
  // Get length
  size_t str_size;
  std::memcpy(&str_size, &data[idx], sizeof(size_t));

  // Get string
  std::vector<unsigned char> svec(&data[idx + sizeof(size_t)],
                                  &data[idx + sizeof(size_t) + str_size]);
  *s = chvec2str(svec);
  return sizeof(size_t) + str_size;
}

/**
 * Puts the next integer from data at index idx into i.
 */
int get_integer(CryptoPP::Integer *i, std::vector<unsigned char> &data,
                int idx)
{
  std::string i_str;
  int n = get_string(&i_str, data, idx);
  *i = CryptoPP::Integer(i_str.c_str());
  return n;
}

// ================================================
// WRAPPERS
// ================================================

void FinalGossip_Message::serialize(std::vector<unsigned char> &data)
{
  data.push_back((char)MessageType::FinalGossip_Message);
  put_string(this->bit_string, data);
}

int FinalGossip_Message::deserialize(std::vector<unsigned char> &data)
{
  assert(data[0] == MessageType::FinalGossip_Message);

  std::string bit_string;
  int n = 1;
  n += get_string(&bit_string, data, n);
  this->bit_string = bit_string;

  return n;
}

void InitialShare_Message::serialize(std::vector<unsigned char> &data)
{
  data.push_back((char)MessageType::InitialShare_Message);

  put_string(std::to_string(this->share_value), data);
}

int InitialShare_Message::deserialize(std::vector<unsigned char> &data)
{
  assert(data[0] == MessageType::InitialShare_Message);

  std::string share_value_string;
  int n = 1;
  n += get_string(&share_value_string, data, n);
  this->share_value = std::stoi(share_value_string);

  return n;
}

/**
 * serialize HMACTagged_Wrapper.
 */
void HMACTagged_Wrapper::serialize(std::vector<unsigned char> &data)
{
  // Add message type.
  data.push_back((char)MessageType::HMACTagged_Wrapper);

  // Add fields.
  put_string(chvec2str(this->payload), data);

  std::string iv = byteblock_to_string(this->iv);
  put_string(iv, data);

  put_string(this->mac, data);
}

/**
 * deserialize HMACTagged_Wrapper.
 */
int HMACTagged_Wrapper::deserialize(std::vector<unsigned char> &data)
{
  // Check correct message type.
  assert(data[0] == MessageType::HMACTagged_Wrapper);

  // Get fields.
  std::string payload_string;
  int n = 1;
  n += get_string(&payload_string, data, n);
  this->payload = str2chvec(payload_string);

  std::string iv;
  n += get_string(&iv, data, n);
  this->iv = string_to_byteblock(iv);

  n += get_string(&this->mac, data, n);
  return n;
}

// ================================================
// KEY EXCHANGE
// ================================================

/**
 * serialize DHPublicValue_Message.
 */
void DHPublicValue_Message::serialize(std::vector<unsigned char> &data)
{
  // Add message type.
  data.push_back((char)MessageType::DHPublicValue_Message);

  // Add fields.
  std::string public_string = byteblock_to_string(this->public_value);
  put_string(public_string, data);
}

/**
 * deserialize DHPublicValue_Message.
 */
int DHPublicValue_Message::deserialize(std::vector<unsigned char> &data)
{
  // Check correct message type.
  assert(data[0] == MessageType::DHPublicValue_Message);

  // Get fields.
  std::string public_string;
  int n = 1;
  n += get_string(&public_string, data, n);
  this->public_value = string_to_byteblock(public_string);
  return n;
}

// ================================================
// OBLIVIOUS TRANSFER
// ================================================

void SenderToReceiver_OTPublicValue_Message::serialize(
    std::vector<unsigned char> &data)
{
  // Add message type.
  data.push_back((char)MessageType::SenderToReceiver_OTPublicValue_Message);

  // Add fields.
  std::string public_integer = byteblock_to_string(this->public_value);
  put_string(public_integer, data);
}

int SenderToReceiver_OTPublicValue_Message::deserialize(
    std::vector<unsigned char> &data)
{
  // Check correct message type.
  assert(data[0] == MessageType::SenderToReceiver_OTPublicValue_Message);

  // Get fields.
  std::string public_integer;
  int n = 1;
  n += get_string(&public_integer, data, n);
  this->public_value = string_to_byteblock(public_integer);
  return n;
}

void ReceiverToSender_OTPublicValue_Message::serialize(
    std::vector<unsigned char> &data)
{
  // Add message type.
  data.push_back((char)MessageType::ReceiverToSender_OTPublicValue_Message);

  // Add fields.
  std::string public_integer = byteblock_to_string(this->public_value);
  put_string(public_integer, data);
}

int ReceiverToSender_OTPublicValue_Message::deserialize(
    std::vector<unsigned char> &data)
{
  // Check correct message type.
  assert(data[0] == MessageType::ReceiverToSender_OTPublicValue_Message);

  // Get fields.
  std::string public_integer;
  int n = 1;
  n += get_string(&public_integer, data, n);
  this->public_value = string_to_byteblock(public_integer);
  return n;
}

void SenderToReceiver_OTEncryptedValues_Message::serialize(
    std::vector<unsigned char> &data)
{
  // Add message type.
  data.push_back((char)MessageType::SenderToReceiver_OTEncryptedValues_Message);

  // Add the # of encrypted values
  int idx = data.size();
  // Resize data to have space for the value of encryptions.size(). Since data
  // contains data of 1 byte each, we just add sizeof(size_t) more elements
  data.resize(idx + sizeof(size_t));
  size_t num_encryptions = encryptions.size();
  // Copy num_encryptions into data[idx]
  std::memcpy(&data[idx], &num_encryptions, sizeof(size_t));

  // Put each encryption.
  for (auto &s : encryptions)
  {
    put_string(s, data);
  }

  // Put each IV. Note that the # of encryptions = # of IVs, so there's no need
  // to add the number for this
  for (auto &iv_block : ivs)
  {
    std::string iv_string = byteblock_to_string(iv_block);
    put_string(iv_string, data);
  }
}

int SenderToReceiver_OTEncryptedValues_Message::deserialize(
    std::vector<unsigned char> &data)
{
  // Check correct message type.
  assert(data[0] == MessageType::SenderToReceiver_OTEncryptedValues_Message);

  // Get length
  size_t num_encryptions;
  std::memcpy(&num_encryptions, &data[1], sizeof(size_t));

  // Get fields. Note that n is the current index into data
  int n = 1 + sizeof(size_t);
  for (int i = 0; i < num_encryptions; i++)
  {
    std::string s;
    n += get_string(&s, data, n);
    encryptions.push_back(s);
  }
  for (int i = 0; i < num_encryptions; i++)
  {
    std::string iv;
    n += get_string(&iv, data, n);
    ivs.push_back(string_to_byteblock(iv));
  }
  return n;
}