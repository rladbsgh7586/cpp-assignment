#include "pch.hpp"

#include "client/client.hpp"

Client::Client(asio::io_context& io, const std::string& host,
               unsigned short port)
    : io_(io), socket_(io), resolver_(io) {
  asio::co_spawn(io_, run(host, port), asio::detached);
}

asio::awaitable<void> Client::run(std::string host, unsigned short port) {
  boost::system::error_code ec;

  auto endpoints = co_await resolver_.async_resolve(
      host, std::to_string(port),
      asio::redirect_error(asio::use_awaitable, ec));
  if (ec) {
    LOG_ERROR("resolve 실패: {}", ec.message());
    co_return;
  }

  co_await asio::async_connect(socket_, endpoints,
                               asio::redirect_error(asio::use_awaitable, ec));
  if (ec) {
    LOG_ERROR("연결 실패: {}", ec.message());
    co_return;
  }
  LOG_INFO("서버 연결됨");

  for (;;) {
    co_await asio::async_read_until(
        socket_, buf_, '\n', asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
      LOG_WARN("수신 종료: {}", ec.message());
      co_return;
    }
    std::istream is(&buf_);
    std::string line;
    std::getline(is, line);
    LOG_INFO("서버: {}", line);
  }
}

void Client::stop() {
  boost::system::error_code ec;
  socket_.close(ec);
}
