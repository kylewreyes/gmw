#pragma once
#include <cstring>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include "../../include-shared/messages.hpp"

class NetworkDriver {
public:
  virtual void listen(int port) = 0;
  virtual void connect(int other_party, std::string address, int port) = 0;
  virtual void disconnect(int other_party) = 0;
  virtual void send(int other_party, std::vector<unsigned char> data) = 0;
  virtual std::vector<unsigned char> read(int other_party) = 0;
  virtual std::string get_remote_info() = 0;
};

class NetworkDriverImpl : public NetworkDriver {
public:
  NetworkDriverImpl();
  void listen(int port);
  void connect(int other_party, std::string address, int port);
  void disconnect(int other_party);
  void send(int other_party, std::vector<unsigned char> data);
  std::vector<unsigned char> read(int other_party);
  std::string get_remote_info();

private:
  int port;
  boost::asio::io_context io_context;
  std::map<int party, std::shared_ptr<boost::asio::ip::tcp::socket> sockets;
};
