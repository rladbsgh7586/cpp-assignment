#include "pch.hpp"

BaseClient::BaseClient(asio::io_context& io, const std::string& host,
                       uint16_t port_base)
    : io_(io), md_socket_(io), oe_socket_(io), resolver_(io) {
  asio::co_spawn(io_, Run(host, port_base), asio::detached);
}

asio::awaitable<void> BaseClient::Run(std::string host, uint16_t port_base) {
  // 주문채널 연결 + 주문결과 수신루프
  co_await ConnectOrderEntry(host, port_base + kOrderEntryPortOffset);
  asio::co_spawn(io_, OeReader(), asio::detached);

  // 시장데이터채널 연결 + 수신루프
  co_await ConnectMarketData(host, port_base + kMarketDataPortOffset);
  asio::co_spawn(io_, MdReader(), asio::detached);

  if (oe_ready_) OnReady();
}

asio::awaitable<void> BaseClient::ConnectOrderEntry(const std::string& host,
                                                    uint16_t port) {
  boost::system::error_code ec;
  auto oe_eps = co_await resolver_.async_resolve(
      host, std::to_string(port),
      asio::redirect_error(asio::use_awaitable, ec));
  if (ec) {
    LOG_ERROR("주문채널 resolve 실패: {}", ec.message());
    co_return;
  }
  co_await asio::async_connect(oe_socket_, oe_eps,
                               asio::redirect_error(asio::use_awaitable, ec));
  if (ec) {
    LOG_ERROR("주문채널 연결 실패: {}", ec.message());
    co_return;
  }

  // 서버가 배정한 client 번호 수신
  int32_t my_id = 0;
  co_await asio::async_read(oe_socket_, asio::buffer(&my_id, sizeof(my_id)),
                            asio::redirect_error(asio::use_awaitable, ec));
  if (ec) {
    LOG_ERROR("client 번호 수신 실패: {}", ec.message());
    co_return;
  }
  oe_ready_ = true;
  OnAssignedId(my_id);
  LOG_INFO("주문채널 연결성공 (배정 #{})", my_id);
}

// 주문결과 수신루프 (OrderEntryPort)
asio::awaitable<void> BaseClient::OeReader() {
  boost::system::error_code ec;
  OrderResultPacket pack;

  for (;;) {
    co_await asio::async_read(oe_socket_, asio::buffer(&pack, sizeof(pack)),
                              asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
      LOG_WARN("주문채널 수신 종료: {}", ec.message());
      co_return;
    }

    // 주문결과패킷 처리
    SideState& s = StateOf(pack.side);
    switch (pack.type) {
      case ResultType::kAccepted:  // 신규주문 등록 확정
        LOG_INFO("등록 완료 {} @{}", SideStr(pack.side), pack.price);
        s.state = OeState::kResting;
        if (s.pending) {  // 등록 대기 중 목표가 변경 -> 재조정(취소 후 재호가)
          Reconcile(pack.side, s.pending);
          s.pending = 0;
        }
        break;

      case ResultType::kCancelled:  // 주문취소 완료 -> 보유주문 없음
        LOG_INFO("주문취소 완료 {} @{}", SideStr(pack.side), pack.price);
        s.state = OeState::kIdle;
        if (s.pending) {  // 보류했던 주문등록
          Reconcile(pack.side, s.pending);
          s.pending = 0;
        }
        break;

      case ResultType::kReject:  // 주문 취소 실패
        LOG_INFO("주문 취소 거부 {} @{} ", SideStr(pack.side), pack.price);
        break;

      case ResultType::kSuccess:  // 주문 체결
        LOG_INFO("주문 체결 {} {}@{}", SideStr(pack.side), pack.qty,
                 pack.price);
        if (pack.price == s.price) {
          s.qty -= pack.qty;
          // 전량 체결 -> 주문 소멸. 다음 목표가 계산 시 신규 등록.
          if (s.qty <= 0) s.state = OeState::kIdle;
        }
        break;
    }
  }
}

asio::awaitable<void> BaseClient::ConnectMarketData(const std::string& host,
                                                    uint16_t port) {
  boost::system::error_code ec;
  auto md_eps = co_await resolver_.async_resolve(
      host, std::to_string(port),
      asio::redirect_error(asio::use_awaitable, ec));
  if (ec) {
    LOG_ERROR("시장데이터채널 resolve 실패: {}", ec.message());
    co_return;
  }
  co_await asio::async_connect(md_socket_, md_eps,
                               asio::redirect_error(asio::use_awaitable, ec));
  if (ec) {
    LOG_ERROR("시장데이터채널 연결 실패: {}", ec.message());
    co_return;
  }
  LOG_INFO("시장데이터채널 연결성공");
}

// 시장데이터(호가창) 수신루프. 파싱 후 파생 클래스 훅(OnOrderBook)에 전달.
// 호가창을 읽지 않으면 서버 송신이 백프레셔로 막히므로 계속 수신해 비운다.
asio::awaitable<void> BaseClient::MdReader() {
  OrderBookPacket pkt;
  boost::system::error_code ec;
  for (;;) {
    co_await asio::async_read(md_socket_, asio::buffer(&pkt, sizeof(pkt)),
                              asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
      LOG_WARN("시장데이터 수신 종료: {}", ec.message());
      co_return;
    }
    Orderbook book = Parse(pkt);
    LOG_DEBUG("호가창 수신{}", FormatBook(book));
    OnOrderBook(book);
  }
}

// 현재 주문상태(OeState)를 반영하여 목표가로 맞춘다.
//   신규 주문 -> 즉시 전송
//   호가 변경 -> 취소 주문 후 Ack 받아 재호가 (pending 에 목표가 보관)
void BaseClient::Reconcile(Side side, int desired) {
  if (!oe_ready_ || desired <= 0) return;
  SideState& s = StateOf(side);

  switch (s.state) {
    case OeState::kIdle: {
      const int qty = OrderQty();  // 파생 클래스가 고정/랜덤 결정
      SendOrder(side, desired, qty);
      s.state = OeState::kWaitAccepted;
      s.price = desired;
      s.qty = qty;
      s.pending = 0;
      LOG_INFO("{} 신규주문 {}@{} (등록대기)", SideStr(side), qty, desired);
      break;
    }

    case OeState::kResting:
      if (s.price == desired) return;  // 이미 목표가에 있음
      SendOrder(side, s.price, 0);     // 취소 (신규는 취소 ACK 후)
      s.state = OeState::kWaitCanceled;
      s.pending = desired;
      LOG_INFO("{} 취소요청 @{} -> 목표가 {}", SideStr(side), s.price, desired);
      break;

    case OeState::kWaitAccepted:
    case OeState::kWaitCanceled:
      s.pending = desired;  // 최신 목표가만 업데이트
      break;
  }
}

void BaseClient::SendOrder(Side side, int price, int qty) {
  OrderPacket pack{side, price, qty};
  boost::system::error_code ec;
  asio::write(oe_socket_, asio::buffer(&pack, sizeof(pack)), ec);
  if (ec) LOG_WARN("주문 송신 실패: {}", ec.message());
}

void BaseClient::Stop() {
  OnStop();
  boost::system::error_code ec;
  md_socket_.close(ec);
  if (ec) LOG_WARN("md_socket_ close failed: {}", ec.message());
  oe_socket_.close(ec);
  if (ec) LOG_WARN("oe_socket_ close failed: {}", ec.message());
}
