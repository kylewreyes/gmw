#include "../include-shared/util.hpp"

#include <crypto++/rng.h>
#include <crypto++/osrng.h>

using namespace CryptoPP;

/**
 * Convert char vec to string.
 */
std::string chvec2str(std::vector<unsigned char> data)
{
  std::string s(data.begin(), data.end());
  return s;
}

/**
 * Convert string to char vec.
 */
std::vector<unsigned char> str2chvec(std::string s)
{
  std::vector<unsigned char> v(s.begin(), s.end());
  return v;
}

/**
 * Convert char vec to string.
 */
std::string hex_encode(std::string s)
{
  std::string res;
  CryptoPP::StringSource(
      s, true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(res)));
  return res;
}

/**
 * Convert string to char vec.
 */
std::string hex_decode(std::string s)
{
  std::string res;
  CryptoPP::StringSource(
      s, true, new CryptoPP::HexDecoder(new CryptoPP::StringSink(res)));
  return res;
}

/**
 * Converts a byte block into an integer.
 */
CryptoPP::Integer byteblock_to_integer(const CryptoPP::SecByteBlock &block)
{
  return CryptoPP::Integer(block, block.size());
}

/**
 * Converts an integer into a byte block.
 */
CryptoPP::SecByteBlock integer_to_byteblock(const CryptoPP::Integer &x)
{
  size_t encodedSize = x.MinEncodedSize(CryptoPP::Integer::UNSIGNED);
  CryptoPP::SecByteBlock bytes(encodedSize);
  x.Encode(bytes.BytePtr(), encodedSize, CryptoPP::Integer::UNSIGNED);
  return bytes;
}

/**
 * Converts a byte block into a string.
 */
std::string byteblock_to_string(const CryptoPP::SecByteBlock &block)
{
  return std::string(block.begin(), block.end());
}
/**
 * Converts a string into a byte block.
 */
CryptoPP::SecByteBlock string_to_byteblock(const std::string &s)
{
  CryptoPP::SecByteBlock block(reinterpret_cast<const byte *>(&s[0]), s.size());
  return block;
}

/**
 * Given a string, it prints its hex representation of the raw bytes it
 * contains. Used for debugging.
 */
void print_string_as_hex(std::string str)
{
  for (int i = 0; i < str.length(); i++)
  {
    std::cout << std::hex << std::setfill('0') << std::setw(2)
              << static_cast<int>(str[i]) << " ";
  }
  std::cout << std::endl;
}

/**
 * Prints contents as integer
 */
void print_key_as_int(const CryptoPP::SecByteBlock &block)
{
  std::cout << byteblock_to_integer(block) << std::endl;
}

/**
 * Prints contents as hex.
 */
void print_key_as_hex(const CryptoPP::SecByteBlock &block)
{
  std::string result;
  CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(result));

  encoder.Put(block, block.size());
  encoder.MessageEnd();

  std::cout << result << std::endl;
}

/**
 * Split a string.
 */
std::vector<std::string> string_split(std::string str, char delimiter)
{
  std::vector<std::string> result;
  // construct a stream from the string
  std::stringstream ss(str);
  std::string s;
  while (std::getline(ss, s, delimiter))
  {
    result.push_back(s);
  }
  return result;
}

std::pair<std::string, int> parse_addr(std::string str)
{
  auto parts = string_split(str, ':');
  return std::pair<std::string, int>(parts[0], std::stoi(parts[1]));
}

/**
 * Generates a random bit.
 */
int generate_bit()
{
  CryptoPP::AutoSeededRandomPool rng;
  return rng.GenerateBit();
}

/**
 * Parse input to a GMW circuit. Each line corresponds to a wire input, and each line will have
 * a party index, followed by a colon, followed by their input. Consider:
 *
 * 2:1
 * 2:0
 * 0:1
 * 1:1
 *
 * This means that:
 *
 * Wire 0: controlled by party 2 with input 1
 * Wire 1: controlled by party 2 with input 0
 * Wire 2: controlled by party 0 with input 1
 * Wire 3: controlled by party 1 with input 1
 */
std::vector<InitialWireInput> parse_input(std::string input_file)
{
  std::string input_str;
  CryptoPP::FileSource(input_file.c_str(), true,
                       new CryptoPP::StringSink(input_str));

  std::vector<InitialWireInput> res;

  std::vector<std::string> lines = string_split(input_str, '\n');
  for (int i = 0; i < lines.size(); i++)
  {
    auto line = lines[i];
    auto parts = string_split(line, ':');

    InitialWireInput wire_input;
    wire_input.party_index = std::stoi(parts[0]);
    wire_input.value = std::stoi(parts[1]);

    res.push_back(wire_input);
  }

  return res;
}

std::vector<std::string> parse_addrs(std::string addr_file)
{
  std::string input_str;
  CryptoPP::FileSource(addr_file.c_str(), true,
                       new CryptoPP::StringSink(input_str));

  return string_split(input_str, '\n');
}