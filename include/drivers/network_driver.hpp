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
  virtual std::vector<unsigned char> read(int other_party) = 0;
  virtual std::vector<unsigned char> socket_read(std::shared_ptr<boost::asio::ip::tcp::socket> sock) = 0;

  virtual void send(int other_party, std::vector<unsigned char> data) = 0;
  virtual void socket_send(std::shared_ptr<boost::asio::ip::tcp::socket> sock, std::vector<unsigned char> data) = 0;

  virtual void listen(int num_connections, int port) = 0;
  virtual void connect(int other_party, std::string address, int port) = 0;
  virtual void disconnect(int other_party) = 0;
};

class NetworkDriverImpl : public NetworkDriver
{
public:
  NetworkDriverImpl();
  ~NetworkDriverImpl();

  std::vector<unsigned char> read(int other_party);
  std::vector<unsigned char> socket_read(std::shared_ptr<boost::asio::ip::tcp::socket> sock);

  void send(int other_party, std::vector<unsigned char> data);
  void socket_send(std::shared_ptr<boost::asio::ip::tcp::socket> sock, std::vector<unsigned char> data);

  void listen(int num_connections, int port);
  void connect(int other_party, std::string address, int port);
  void disconnect(int other_party);

  // When doing sending, we want to specify who exactly we want to be sending content to. This
  // is why we have a map going from party index to the associated socket.
  std::unordered_map<int, std::shared_ptr<boost::asio::ip::tcp::socket>> send_conns;

  // When receiving data, we don't have to know who we're talking to. That's why we just keep a
  // set of connections.
  std::unordered_set<std::shared_ptr<boost::asio::ip::tcp::socket>> recv_conns;

private:
  // Sharing io_context's allow for performance benefit when doing async IO
  boost::asio::io_context io_context;
};
