#include "pch.hpp"

#include "server/server.hpp"

Server::Server(asio::io_context& io, unsigned short port_base)
    : io_(io),
      md_acceptor_(
          io, tcp::endpoint(tcp::v4(), static_cast<unsigned short>(
                                           port_base + kMarketDataPortOffset))),
      oe_acceptor_(io, tcp::endpoint(tcp::v4(),
                                     static_cast<unsigned short>(
                                         port_base + kOrderEntryPortOffset))) {
  InitBook();
  asio::co_spawn(io_, MdListener(), asio::detached);
  asio::co_spawn(io_, OeListener(), asio::detached);
}

// ---------------------------------------------------------------------------
// 시장데이터 채널 (9000) orderbook broadcaset
// ---------------------------------------------------------------------------
asio::awaitable<void> Server::MdListener() {
  try {
    for (;;) {
      tcp::socket socket =
          co_await md_acceptor_.async_accept(asio::use_awaitable);
      int id = next_md_id_++;
      auto s = std::make_shared<MdSession>(std::move(socket), id);
      md_sessions_[id] = s;
      LOG_INFO("client #{} 연결 (현재 {})", id, md_sessions_.size());

      // 최초 연결 시 현재 호가창을 1회 즉시 전송
      s->pending = engine_.ExtractOrderBook();
      s->has_pending = true;
      s->writing = true;
      asio::co_spawn(io_, MdWriter(s), asio::detached);
      asio::co_spawn(io_, MdWatch(s), asio::detached);
    }
  } catch (const std::exception&) {
    // acceptor 닫힘(Stop) -> 수락 종료
  }
}
void Server::BroadcastOrderBook() {
  OrderBookPacket pkt = engine_.ExtractOrderBook();
  LOG_DEBUG("호가창 발행{}", FormatBook(Parse(pkt)));
  for (auto& [id, s] : md_sessions_) {
    s->pending = pkt;
    s->has_pending = true;
    if (!s->writing) {
      s->writing = true;
      asio::co_spawn(io_, MdWriter(s), asio::detached);
    }
  }
}

// 최신 스냅샷 하나만 항상 발행시점의 최신 orderbook 전송할수있도록
asio::awaitable<void> Server::MdWriter(std::shared_ptr<MdSession> s) {
  boost::system::error_code ec;
  while (s->has_pending) {
    OrderBookPacket pkt = s->pending;
    s->has_pending = false;
    co_await asio::async_write(s->socket, asio::buffer(&pkt, sizeof(pkt)),
                               asio::redirect_error(asio::use_awaitable, ec));
    if (ec) break;
  }
  s->writing = false;
}

// 클라이언트와의 연결끊김 감지
asio::awaitable<void> Server::MdWatch(std::shared_ptr<MdSession> s) {
  char buf[64];
  boost::system::error_code ec;
  for (;;) {
    co_await s->socket.async_read_some(
        asio::buffer(buf), asio::redirect_error(asio::use_awaitable, ec));
    if (ec) break;  // 연결끊김
  }
  md_sessions_.erase(s->id);
  LOG_INFO("client #{} 연결 종료 (현재 {})", s->id, md_sessions_.size());
}

// ---------------------------------------------------------------------------
// 주문 채널 (9001) -> 매칭엔진
// 단일스레드 코루틴, 주문을 받는순서대로 처리하므로 FIFO 보장
// ---------------------------------------------------------------------------
asio::awaitable<void> Server::OeListener() {
  boost::system::error_code ec;
  try {
    for (;;) {
      tcp::socket socket =
          co_await oe_acceptor_.async_accept(asio::use_awaitable);
      int id = next_oe_id_++;
      auto s = std::make_shared<OeSession>(std::move(socket), id);

      // 핸드셰이크: 배정한 client 번호를 먼저 전송 (4-byte int32).
      int32_t id_msg = id;
      co_await asio::async_write(s->socket,
                                 asio::buffer(&id_msg, sizeof(id_msg)),
                                 asio::redirect_error(asio::use_awaitable, ec));
      if (ec) continue;  // 연결실패

      oe_sessions_[id] = s;
      LOG_INFO("주문 client #{} 연결 (현재 {})", id, oe_sessions_.size());
      asio::co_spawn(io_, OeReader(s), asio::detached);
    }
  } catch (const std::exception&) {
    // acceptor 닫힘(Stop) -> 수락 종료
  }
}

asio::awaitable<void> Server::OeReader(std::shared_ptr<OeSession> s) {
  boost::system::error_code ec;
  OrderPacket order;
  for (;;) {
    co_await asio::async_read(s->socket, asio::buffer(&order, sizeof(order)),
                              asio::redirect_error(asio::use_awaitable, ec));
    if (ec) break;

    bool dirty = false;
    std::vector<OrderResult> execs =
        engine_.PushOrder(s->client_id, order, dirty);
    // 주문 결과는 즉시 해당 클라이언트로 전송
    SendOrderResults(execs);

    // 호가창 변경시 OrderBook 발행
    if (dirty) {
      BroadcastOrderBook();
    }
  }
  oe_sessions_.erase(s->client_id);
  LOG_INFO("주문 client #{} 연결 종료 (현재 {})", s->client_id,
           oe_sessions_.size());
}

// OrderResult를 받을 클라이언트를 세션에서 찾고 결과 전송
void Server::SendOrderResults(const std::vector<OrderResult>& results) {
  for (const auto& r : results) {
    auto it = oe_sessions_.find(r.client_id);
    if (it == oe_sessions_.end()) continue;  // 세션이 이미 종료됨
    auto& s = it->second;
    s->wq.push_back(OrderResultPacket{r.type, r.side, r.price, r.qty});
    if (!s->writing) {
      s->writing = true;
      asio::co_spawn(io_, OeWriter(s), asio::detached);
    }
  }
}

asio::awaitable<void> Server::OeWriter(std::shared_ptr<OeSession> s) {
  boost::system::error_code ec;
  while (!s->wq.empty()) {
    OrderResultPacket e = s->wq.front();
    co_await asio::async_write(s->socket, asio::buffer(&e, sizeof(e)),
                               asio::redirect_error(asio::use_awaitable, ec));
    if (ec) break;
    s->wq.pop_front();
  }
  s->writing = false;
}

void Server::Stop() {
  md_acceptor_.close();
  oe_acceptor_.close();
  for (auto& [id, s] : md_sessions_) s->socket.close();
  for (auto& [id, s] : oe_sessions_) s->socket.close();
  md_sessions_.clear();
  oe_sessions_.clear();
}

// 초기 호가창 주문 생성
void Server::InitBook() {
  bool dummy = false;
  for (int i = 0; i < kNumLayers; ++i) {
    engine_.PushOrder(kSeedClientId, {Side::kBuy, kSeedMid - 1 - i, kSeedQty},
                      dummy);
    engine_.PushOrder(kSeedClientId, {Side::kSell, kSeedMid + 1 + i, kSeedQty},
                      dummy);
  }
  LOG_INFO("seed 호가창: bid {}~{} / ask {}~{} (qty {})", kSeedMid - kNumLayers,
           kSeedMid - 1, kSeedMid + 1, kSeedMid + kNumLayers, kSeedQty);
}