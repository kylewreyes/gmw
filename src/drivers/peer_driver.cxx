#include <thread>
#include <future>

#include "../../include/drivers/peer_driver.hpp"
#include "../../include/drivers/ot_driver.hpp"

PeerDriver::PeerDriver(int my_party, std::vector<std::string> addrs)
{
    this->my_party = my_party;
    this->addrs = addrs;

    this->crypto_driver = std::make_shared<CryptoDriver>();

    // Establish a connection to every other party (except ourselves)
    for (int i = 0; i < addrs.size(); i++) {
        if (i == my_party) {
            continue;
        }

        this->network_drivers[i] = NetworkDriverImpl();
        // HandleKeyExchange with party i
    }


    // std::shared_ptr<NetworkDriver> network_driver =
    // std::make_shared<NetworkDriverImpl>();
    // network_driver->listen(addrs[my_party]);
    // std::shared_ptr<CryptoDriver> crypto_driver =
    //     std::make_shared<CryptoDriver>();
}

template <typename T, typename K>
std::shared_future<T> PeerDriver::start_in_thread(int other, SendType send_type, K payload)
{
    return std::async(std::launch::async, PeerDriver::do_start_in_thread, other, send_type, payload);
}

// TODO(neil): both T and K need to implement serializable
template <typename T, typename K>
std::shared_future<T> PeerDriver::do_start_in_thread(int other, SendType send_type, K payload)
{
    int should_send_first = this->my_party < other;
    // Just use the default network connection.
    auto network_driver = this->network_drivers[other];

    if (send_type == SEND_OT)
    {
        // TODO: (plumb this)
        OTDriver OT_driver(network_driver, this->crypto_driver, 0, 0);

        if (should_send_first)
        {
            OT_driver.OT_send("foobar");
        }
        else
        {
            auto ret = OT_driver.OT_recv(0);
            return ret;
        }
    }
    else
    {

        if (should_send_first)
        {
            network_driver.send(payload);
        }
        else
        {
            return network_driver.read();
        }
    }
}