#pragma once

#include <fstream>
#include <stdio.h>
#include <string>
#include <vector>

#include <crypto++/cryptlib.h>
#include <crypto++/integer.h>
#include <crypto++/secblock.h>

// ================================================
// REGULAR CIRCUIT
// ================================================

namespace GateType {
enum T { AND_GATE = 1, XOR_GATE = 2, NOT_GATE = 3 };
};

struct Gate {
  GateType::T type;
  int lhs;    // wire index of lhs
  int rhs;    // wire index of rhs
  int output; // wire index of output
};

struct Circuit {
  int num_gate, num_wire, input_length,
      output_length;
  std::vector<Gate> gates;
};
Circuit parse_circuit(std::string filename);

// ================================================
// GARBLED CIRCUIT
// ================================================

struct GarbledWire {
  CryptoPP::SecByteBlock value;
};

struct GarbledGate {
  std::vector<CryptoPP::SecByteBlock> entries;
};

struct GarbledLabels {
  std::vector<GarbledWire> zeros;
  std::vector<GarbledWire> ones;
};

struct GarbledCircuit {
  std::vector<GarbledWire> garbled_wires;
  std::vector<GarbledGate> garbled_gates;
};
