#include "pch.hpp"

#include "server/server.hpp"

Server::Server(asio::io_context& io, unsigned short port)
    : io_(io),
      acceptor_(io, tcp::endpoint(tcp::v4(), port)),
      start_(std::chrono::steady_clock::now()) {
  asio::co_spawn(io_, listener(), asio::detached);
  asio::co_spawn(io_, ticker(), asio::detached);
}

// accept 루프
asio::awaitable<void> Server::listener() {
  try {
    for (;;) {
      tcp::socket socket = co_await acceptor_.async_accept(asio::use_awaitable);

      if (sessions_.size() >= kMaxClients) {
        LOG_WARN("연결 거부: 최대 {}개 초과", kMaxClients);
        socket.close();
        continue;
      }
      int id = next_id_++;
      auto s = std::make_shared<Session>(std::move(socket), id);
      sessions_[id] = s;
      LOG_INFO("client #{} 연결 (현재 {})", id, sessions_.size());
      asio::co_spawn(io_, watch(s), asio::detached);
    }
  } catch (const std::exception&) {
    // acceptor 닫힘(stop) -> 수락 종료
  }
}

// 1초마다 elapsed broadcast
asio::awaitable<void> Server::ticker() {
  asio::steady_timer timer(io_);
  for (;;) {
    timer.expires_after(std::chrono::seconds(1));
    co_await timer.async_wait(asio::use_awaitable);

    auto secs = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - start_)
                    .count();
    std::string msg = "elapsed: " + std::to_string(secs) + "s\n";

    // 송신 도중 sessions_ 를 erase하는 시나리오를 방지하기 위해 snap활용하여 broadcaet
    std::vector<std::shared_ptr<Session>> snap;
    snap.reserve(sessions_.size());
    for (auto& [id, s] : sessions_) snap.push_back(s);

    for (auto& s : snap) {
      if (!s->socket.is_open()) continue;
      try {
        co_await asio::async_write(s->socket, asio::buffer(msg),
                                   asio::use_awaitable);
      } catch (const std::exception&) {
      }
    }
    LOG_DEBUG("broadcast elapsed={}s (clients={})", secs, sessions_.size());
  }
}

// 세션별 끊김 감지
asio::awaitable<void> Server::watch(std::shared_ptr<Session> s) {
  char buf[64];
  boost::system::error_code ec;
  for (;;) {
    // error code로 끊김 감지
    co_await s->socket.async_read_some(
        asio::buffer(buf), asio::redirect_error(asio::use_awaitable, ec));
    if (ec) break;
  }
  // 소켓은 Session 소멸 시 RAII 로 닫힌다
  sessions_.erase(s->id);
  LOG_INFO("client #{} 연결 종료 (현재 {})", s->id, sessions_.size());
}

void Server::stop() {
  acceptor_.close();
  for (auto& [id, s] : sessions_) {
    s->socket.close();
  }

  sessions_.clear();
}
