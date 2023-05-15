#pragma once
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <cstring>
#include <iostream>
#include <unordered_set>

#include "../../include-shared/messages.hpp"

class NetworkDriver
{
public:
  virtual std::vector<unsigned char> socket_read(std::shared_ptr<boost::asio::ip::tcp::socket> sock) = 0;
  virtual void socket_send(std::shared_ptr<boost::asio::ip::tcp::socket> sock, std::vector<unsigned char> data) = 0;

  virtual std::shared_ptr<boost::asio::ip::tcp::socket> listen(int port) = 0;
  virtual std::shared_ptr<boost::asio::ip::tcp::socket> connect(int other_party, std::string address, int port) = 0;
  virtual void disconnect(int other_party) = 0;
};

class NetworkDriverImpl : public NetworkDriver
{
public:
  NetworkDriverImpl();
  ~NetworkDriverImpl();

  std::vector<unsigned char> socket_read(std::shared_ptr<boost::asio::ip::tcp::socket> sock);
  void socket_send(std::shared_ptr<boost::asio::ip::tcp::socket> sock, std::vector<unsigned char> data);

  std::shared_ptr<boost::asio::ip::tcp::socket> listen(int port);
  std::shared_ptr<boost::asio::ip::tcp::socket> connect(int other_party, std::string address, int port);
  void disconnect(int other_party);

private:
  // Sharing io_context's allow for performance benefit when doing async IO
  boost::asio::io_context io_context;
};
