#pragma once

#include "pch.hpp"

class Client {
 public:
  Client(asio::io_context& io, const std::string& host, unsigned short port);
  void stop();

 private:
  asio::awaitable<void> run(std::string host, unsigned short port);

  asio::io_context& io_;
  tcp::socket socket_;
  tcp::resolver resolver_;
  asio::streambuf buf_;
};
