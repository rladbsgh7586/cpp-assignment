// 단일 포트 TCP 서버 (C++20 coroutine 버전).
// 참고예제:
// https://www.boost.org/doc/libs/latest/doc/html/boost_asio/example/cpp20/coroutines/echo_server.cpp
#pragma once

#include "pch.hpp"

#include "server/session.hpp"

class Server {
 public:
  Server(asio::io_context& io, unsigned short port);
  void stop();

 private:
  asio::awaitable<void> listener();
  asio::awaitable<void> ticker();
  asio::awaitable<void> watch(std::shared_ptr<Session> s);

  static constexpr int kMaxClients = 3;

  asio::io_context& io_;
  tcp::acceptor acceptor_;
  std::chrono::steady_clock::time_point start_;
  std::map<int, std::shared_ptr<Session>> sessions_;
  int next_id_ = 1;
};
