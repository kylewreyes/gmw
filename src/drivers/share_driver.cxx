#include <crypto++/osrng.h>
#include <cstdlib>
#include <iostream>

#include "../../include/drivers/share_driver.hpp"

using namespace CryptoPP;

/**
 * Constructor
 */
ShareDriver::ShareDriver(int my_party, int num_parties)
{
    this->my_party = my_party;
    this->num_parties = num_parties;
}

/**
 * If this party owns a particular wire, they need to generate secret shares for all other parties,
 * as well as themselves. These values all need to XOR to be target.
 */
std::vector<int> ShareDriver::generate_shares(int target)
{
    // We need to initialize a SecByteBlock with a number of bytes. If we have one bit per
    // num_parties, then we need CEIL(num_parties / 8) bytes.
    std::vector<int> shares(this->num_parties, 0);

    // Initialize to zero, since 1 would invert the first value.
    int xor_other_parties = 0;

    CryptoPP::AutoSeededRandomPool rng;

    for (int i = 0; i < this->num_parties; i++)
    {
        if (i == this->my_party)
        {
            continue;
        }

        int bit = rng.GenerateBit();
        shares[i] = bit;
        xor_other_parties ^= bit;
    }

    // Finally, generate our share
    shares[this->my_party] = (xor_other_parties ^ target);

    std::cout << "Generating share for target: " << target << std::endl;
    for (int share : shares)
    {
        std::cout << "Got share to be " << share << std::endl;
    }

    return shares;
}