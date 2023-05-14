#include "../../include/drivers/network_driver.hpp"

#include <stdexcept>
#include <vector>

using namespace boost::asio;
using ip::tcp;

/**
 * Constructor. Sets up IO context and socket.
 */
NetworkDriverImpl::NetworkDriverImpl(
    std::unordered_map<std::string, int> addresses)
    : addresses{addresses}, io_context() {}

NetworkDriverImpl::~NetworkDriverImpl() { io_context.stop(); }

/**
 * Listen on the given port at localhost.
 * @param port Port to listen on.
 */
void NetworkDriverImpl::listen(int port) {
  tcp::acceptor acceptor(this->io_context, tcp::endpoint(tcp::v4(), port));
  auto s = std::make_shared<tcp::socket>(io_context);
  acceptor.accept(*s);
  std::string remote_info = s->remote_endpoint().address().to_string() + ":" +
                            std::to_string(s->remote_endpoint().port());
  int party_num = addresses[remote_info];
  sockets[party_num] = s;
}

/**
 * Connect to the given address and port.
 * @param address Address to connect to.
 * @param port Port to conect to.
 */
void NetworkDriverImpl::connect(int other_party, std::string address,
                                int port) {
  if (address == "localhost") address = "127.0.0.1";

  std::shared_ptr<boost::asio::ip::tcp::socket> s;

  auto it = sockets.find(other_party);
  if (it != sockets.end()) {
    s = it->second;
  } else {
    s = std::make_shared<tcp::socket>(io_context);
    sockets[other_party] = s;
  }

  s->connect(tcp::endpoint(
      boost::asio::ip::address::from_string(address), port));
}

/**
 * Disconnect graceefully.
 */
void NetworkDriverImpl::disconnect(int other_party) {
  sockets[other_party]->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
  sockets[other_party]->close();
  sockets.erase(other_party);
}

/**
 * Sends a fixed amount of data by sending length first.
 * @param data Bytes of data to send.
 */
void NetworkDriverImpl::send(int other_party, std::vector<unsigned char> data) {
  int length = htonl(data.size());
  boost::asio::write(*sockets[other_party],
                     boost::asio::buffer(&length, sizeof(int)));
  boost::asio::write(*sockets[other_party], boost::asio::buffer(data));
}

/**
 * Receives a fixed amount of data by receiving length first.
 * @return std::vector<unsigned char> data read.
 * @throws error when eof.
 */
std::vector<unsigned char> NetworkDriverImpl::read(int other_party) {
  // read length
  int length;
  boost::system::error_code error;
  boost::asio::read(*sockets[other_party],
                    boost::asio::buffer(&length, sizeof(int)),
                    boost::asio::transfer_exactly(sizeof(int)), error);
  if (error) {
    throw std::runtime_error("Received EOF.");
  }
  length = ntohl(length);

  // read message
  std::vector<unsigned char> data;
  data.resize(length);
  boost::asio::read(*sockets[other_party], boost::asio::buffer(data),
                    boost::asio::transfer_exactly(length), error);
  if (error) {
    throw std::runtime_error("Received EOF.");
  }
  return data;
}

/**
 * Get socket info as string.
 */
std::string NetworkDriverImpl::get_remote_info(int other_party) {
  return sockets[other_party]->remote_endpoint().address().to_string() + ":" +
         std::to_string(sockets[other_party]->remote_endpoint().port());
}
