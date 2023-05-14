#pragma once

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <crypto++/cryptlib.h>
#include <crypto++/files.h>
#include <crypto++/filters.h>
#include <crypto++/hex.h>
#include <crypto++/integer.h>
#include <crypto++/misc.h>
#include <crypto++/sha.h>
#include <crypto++/rng.h>

// String <=> Vec<char>.
std::string chvec2str(std::vector<unsigned char> data);
std::vector<unsigned char> str2chvec(std::string s);

// String <=> Hex.
std::string hex_encode(std::string s);
std::string hex_decode(std::string s);

// SecByteBlock <=> Integer.
CryptoPP::Integer byteblock_to_integer(const CryptoPP::SecByteBlock &block);
CryptoPP::SecByteBlock integer_to_byteblock(const CryptoPP::Integer &x);

// SecByteBlock <=> string.
std::string byteblock_to_string(const CryptoPP::SecByteBlock &block);
CryptoPP::SecByteBlock string_to_byteblock(const std::string &s);

// Printers.
void print_string_as_hex(std::string str);
void print_key_as_int(const CryptoPP::SecByteBlock &block);
void print_key_as_hex(const CryptoPP::SecByteBlock &block);

// Randomness.
int generate_bit();

// Splitter.
std::vector<std::string> string_split(std::string str, char delimiter);

// Input struct from a file
struct InitialWireInput
{
    int party_index;
    int value;
};

// Input parser.
std::vector<InitialWireInput> parse_input(std::string input_file);

// Address parser
std::vector<std::string> parse_addrs(std::string addr_file);
