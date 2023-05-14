#include <iostream>

#include "circuit.hpp"
#include "crypto++/sha.h"

/*
 * Parse circuit from file in Bristol format.
 */
Circuit parse_circuit(std::string filename) {
  Circuit circuit;

  int garbler_input_length;
  int evaluator_input_length;

  // Open file, scan header.
  FILE *f = fopen(filename.c_str(), "r");
  (void)fscanf(f, "%d%d\n", &circuit.num_gate, &circuit.num_wire);
  (void)fscanf(f, "%d%d%d\n", &garbler_input_length,
               &evaluator_input_length, &circuit.output_length);
  (void)fscanf(f, "\n");

  // Compute the input_length from the garbler and evaluator length, so that we don't have to
  // change the existing input files.
  circuit.input_length = garbler_input_length + evaluator_input_length;

  // Scan gates.
  circuit.gates.resize(circuit.num_gate);
  int tmp, lhs, rhs, output;
  char str[10];
  for (int i = 0; i < circuit.num_gate; ++i) {
    (void)fscanf(f, "%d", &tmp);
    if (tmp == 2) {
      (void)fscanf(f, "%d%d%d%d%s", &tmp, &lhs, &rhs, &output, str);
      if (str[0] == 'A')
        circuit.gates[i] = {GateType::AND_GATE, lhs, rhs, output};
      else if (str[0] == 'X')
        circuit.gates[i] = {GateType::XOR_GATE, lhs, rhs, output};
    } else if (tmp == 1) {
      (void)fscanf(f, "%d%d%d%s", &tmp, &lhs, &output, str);
      circuit.gates[i] = {GateType::NOT_GATE, lhs, 0, output};
    }
  }

  return circuit;
}
