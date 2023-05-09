# GMW Protocol

The Goldreich-Micali-Wigderson (GMW) protocol is a protocol allows for secure multi-party computation (MPC) on any arbitrary boolean circuit. The implementation of this protocol is largely based off of [Yao's](https://brownappliedcryptography.github.io/projects/yaos) assignment from CSCI 1515 at Brown. For more information about this protocol, please see Section 3.2 of [A Pragmatic Introduction to Secure Multi-Party Computation](https://securecomputation.org/docs/pragmaticmpc.pdf). Parts of our implementation was also modeled off of Section 2.1 of [Secure Multi-Party Computation of Boolean Circuits
with Applications to Privacy in On-Line Marketplaces](https://eprint.iacr.org/2011/257.pdf) 

This is Kyle Reyes and Neil Ramaswamy's final project for CSCI 1515: Applied Cryptography.

## Usage

To build this project, `cd` into the `build` directory and run `cmake ..`. This only needs to be run once. Afterwards, the project can be compiled by running `make` while in the `build` directory.
