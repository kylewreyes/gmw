#pragma once

#include <cstdlib>
#include <vector>
#include <string>
#include <sys/ioctl.h>

class ShareDriver
{
public:
  ShareDriver(int my_party, int num_parties);

  // generate_shares creates num_parties secret shares such that they all XOR to be target.
  // The secret share for my_party must be equal to the XOR of target and all the other shares.
  std::vector<int> generate_shares(int target);

private:
  // The party-index of this party, from [0, num_parties)
  int my_party;
  // The total number of parties participating in this MPC
  int num_parties;
};
