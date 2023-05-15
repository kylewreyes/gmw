#include "../../include/drivers/network_driver.hpp"

#include <stdexcept>
#include <vector>

using namespace boost::asio;
using ip::tcp;

/**
 * Constructor. Sets up IO context and socket.
 */
NetworkDriverImpl::NetworkDriverImpl() : io_context() {}

NetworkDriverImpl::~NetworkDriverImpl() { io_context.stop(); }

/**
 * Listen on the given port at localhost.
 *
 * @param port Port to listen on.
 */
std::shared_ptr<tcp::socket> NetworkDriverImpl::listen(int port)
{
  tcp::acceptor acceptor(this->io_context, tcp::endpoint(tcp::v4(), port));
  auto s = std::make_shared<tcp::socket>(io_context);
  acceptor.accept(*s);
  std::string remote_info = s->remote_endpoint().address().to_string() + ":" +
                            std::to_string(s->remote_endpoint().port());
  std::cout << "Party listening on port " << port << "got connection from " << remote_info << std::endl;
  return s;
}

/**
 * Connect to the given address and port. Must be connecting to a party greater than ours.
 *
 * @param address Address to connect to.
 * @param port Port to conect to.
 */
std::shared_ptr<tcp::socket> NetworkDriverImpl::connect(int other_party, std::string address, int port)
{
  auto s = std::make_shared<tcp::socket>(io_context);

  while (true)
  {
    try
    {
      s->connect(tcp::endpoint(boost::asio::ip::address::from_string(address), port));
      return s;
    }
    catch (boost::wrapexcept<boost::system::system_error> &e)
    {
      std::cout << "Couldn't connect to party " << other_party << ". Retrying in 3 seconds." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(3));
    }
  }
}

/**
 * Disconnect graceefully.
 */
void NetworkDriverImpl::disconnect(int other_party)
{
  // sockets[other_party]->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
  // sockets[other_party]->close();
  // sockets.erase(other_party);
  throw std::runtime_error("not yet implemented!");
}

void NetworkDriverImpl::socket_send(std::shared_ptr<boost::asio::ip::tcp::socket> sock, std::vector<unsigned char> data)
{
  int length = htonl(data.size());
  boost::asio::write(*sock,
                     boost::asio::buffer(&length, sizeof(int)));
  boost::asio::write(*sock, boost::asio::buffer(data));
}

/**
 * Receives a fixed amount of data by receiving length first.
 * @return std::vector<unsigned char> data read.
 * @throws error when eof.
 */
std::vector<unsigned char> NetworkDriverImpl::socket_read(std::shared_ptr<boost::asio::ip::tcp::socket> sock)
{
  // read length
  int length;
  boost::system::error_code error;
  boost::asio::read(*sock,
                    boost::asio::buffer(&length, sizeof(int)),
                    boost::asio::transfer_exactly(sizeof(int)), error);
  if (error)
  {
    throw std::runtime_error("Received EOF.");
  }
  length = ntohl(length);

  // read message
  std::vector<unsigned char> data;
  data.resize(length);
  boost::asio::read(*sock, boost::asio::buffer(data),
                    boost::asio::transfer_exactly(length), error);
  if (error)
  {
    throw std::runtime_error("Received EOF.");
  }
  return data;
}