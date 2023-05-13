#include <vector>

#include "../../include/drivers/network_driver.hpp"
#include "../../include/drivers/crypto_driver.hpp"

/*


For the initial stage:

In a background thread,

- We want to communicate with a certain party (or we can be passed in a network driver)
- We want to send them an obj (or a set of objs), and we want to receive a set of objs back

- The thread should terminate when that is over


For subsequent stages with OT:

- OT driver gets a network connection
- We can use the network connection that we have established with the server previously
- We probably want this OT to happen in a background thread

- In OT, we have the concept of a sender and a receiver which makes coordination easy.


func startInThread(int other, send_type, payload) thread {
    if send_type == OT:
        if me < other:
            ret = OT_send(payload)
        pass
            ret = OT_receive()

        return ret
    else:
        if me < other:
            send()
            ret = receive()
        else:
            ret = receive()
            send()

        return ret
}


// Initial secret sharing API

std::vector threads;
for payload_i in payloads:
    threads.push_back(startInThread<PayloadType>(me, i, payload))

for thread in threads:
    thread.join()



// Resharing. Computing gate g with g_l, g_r, and g_out.

// Party i needs to do an OT with every other party

std::vector threads;
for other_party in other_parties:
    threads.push_back(startInThread(other_party, OT, my_ot_thing))
*/

enum SendType
{
    SEND_OT,
    SEND_AE
};

class PeerDriver
{
public:
    /*
     * PeerDriver provides an API to asynchronously communicate with parties in a multiparty
     * computation. It can establish a TCP connection and provide async communication with addr[i],
     * where addrs[i] should correspond to the address of the i'th party in the computation.
     * A connection is not made to oneself, i.e. addrs[my_party] is not connected to.
     *
     * Consider the following:
     *
     * PeerDriver pd(2, {"localhost:1000", "localhost:1001", "localhost:1002"})
     *
     * In this example, party 2 will connect to localhost:1000 and localhost:10001, but not
     * localhost:1002.
     */
    PeerDriver(int my_party, std::vector<std::string> addrs);

    /*
     * startInThread enables asynchronous, one-time bidirectional communication between two parties.
     *
     * T is the type of the expected return value from the communication; K is the type of the
     * payload that is sent to the other party.
     *
     * Example usage:
     *
     * auto f = PeerDriver::startInThread(1, SEND_OT, c_j)
     * f.get() // 1's response value.
     */
    template <typename T, typename K>
    std::future<T> start_in_thread(int other, SendType send_type, K payload);

private:
    // The party-index of this party, from [0, num_parties)
    int my_party;
    // The addresses of the peers in the computation, including our own address.
    std::vector<std::string> addrs;

    // The secret keys for each party, excluding my_party
    std::vector<std::tuple<CryptoPP::SecByteBlock, CryptoPP::SecByteBlock>> keys;

    // A vector of NetworkDriver's to communiate with peers.
    // There is no NetworkDriver associated with network_drivers[this->my_party].
    std::vector<NetworkDriver> network_drivers;

    // I think we can use one CryptoDriver instance here
    std::shared_ptr<CryptoDriver> crypto_driver;

    // A helper function for startInThread
    template <typename T, typename K>
    std::future<T> do_start_in_thread(int other, SendType send_type, K payload);
};