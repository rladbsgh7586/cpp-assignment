#include "pch.hpp"

#include "client/client.hpp"

#include "client/strategy.hpp"

namespace {
// 주문 수량
inline constexpr int kOrderQty = 10;
}  // namespace

Client::Client(asio::io_context& io, const std::string& host,
               uint16_t port_base, int q, int n)
    : io_(io), md_socket_(io), oe_socket_(io), resolver_(io), q_(q), n_(n) {
  asio::co_spawn(io_, Run(host, port_base), asio::detached);
}

asio::awaitable<void> Client::Run(std::string host, uint16_t port_base) {
  // 주문채널 연결 (9001)
  co_await ConnectOrderEntry(host, port_base + kOrderEntryPortOffset);
  // 주문결과 수신루프
  asio::co_spawn(io_, OeReader(), asio::detached);

  // 시장데이터채널 연결 (9000)
  co_await ConnectMarketData(host, port_base + kMarketDataPortOffset);
  // 시장데이터 수신루프
  asio::co_spawn(io_, MdReader(), asio::detached);
}

asio::awaitable<void> Client::ConnectOrderEntry(const std::string& host,
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

  // client 번호 수신 -> 로그 태그 갱신
  int32_t my_id = 0;
  co_await asio::async_read(oe_socket_, asio::buffer(&my_id, sizeof(my_id)),
                            asio::redirect_error(asio::use_awaitable, ec));
  if (ec) {
    LOG_ERROR("client 번호 수신 실패: {}", ec.message());
    co_return;
  }
  logger::SetInstanceId(my_id);
  oe_ready_ = true;
  LOG_INFO("주문채널 연결성공 #{}", my_id);
}

// 주문결과 수신루프 (OrderEntryPort 9000)
asio::awaitable<void> Client::OeReader() {
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
          // 전량 체결 -> 주문 소멸. 다음 orderbook 기반 최신 best price 계산
          if (s.qty <= 0) s.state = OeState::kIdle;
        }
        break;
    }
  }
}

asio::awaitable<void> Client::ConnectMarketData(const std::string& host,
                                                  uint16_t port) {
  boost::system::error_code ec;
  auto md_eps = co_await resolver_.async_resolve(
      host, std::to_string(port),
      asio::redirect_error(asio::use_awaitable, ec));
  if (ec) {
    LOG_ERROR("시장데이터채널  실패: {}", ec.message());
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

// 마켓데이터 수신루프 (MarketDataPort 9001)
asio::awaitable<void> Client::MdReader() {
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

    // Q/N에 기반하여 최우선 매수호가/매도호가 계산
    auto best = strategy::CalcBestPrice(book, q_, n_);
    // best가 없는경우 -> 유동성 부족으로 양쪽 최우선호가가 충족되지 않을때
    if (best) {
      Reconcile(Side::kBuy, best->bid_price);
      Reconcile(Side::kSell, best->ask_price);
    }
  }
}

// 현재 주문상태 (OeState) 반영하여 정책 결정
// 신규 주문 -> 즉시 처리
// 호가 변경 -> 취소 주문후 Ack 받아서 주문
void Client::Reconcile(Side side, int desired) {
  if (!oe_ready_ || desired <= 0) return;
  SideState& s = StateOf(side);

  switch (s.state) {
    case OeState::kIdle:
      SendOrder(side, desired, kOrderQty);
      s.state = OeState::kWaitAccepted;
      s.price = desired;
      s.qty = kOrderQty;
      s.pending = 0;
      LOG_INFO("{} 신규주문 @{} (등록대기)", SideStr(side), desired);
      break;

    case OeState::kResting:
      if (s.price == desired) return;  // 이미 목표가에 있음
      SendOrder(side, s.price, 0);    // 취소 (신규는 취소 ACK 후)
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

void Client::SendOrder(Side side, int price, int qty) {
  OrderPacket pack{side, price, qty};
  boost::system::error_code ec;
  asio::write(oe_socket_, asio::buffer(&pack, sizeof(pack)), ec);
  if (ec) LOG_WARN("주문 송신 실패: {}", ec.message());
}

void Client::Stop() {
  boost::system::error_code ec;
  md_socket_.close(ec);
  if (ec) {
    LOG_WARN("md_socket_ close failed: {}", ec.message());
  }
  oe_socket_.close(ec);
  if (ec) {
    LOG_WARN("oe_socket_ close failed: {}", ec.message());
  }
}
