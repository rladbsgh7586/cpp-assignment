#include "pch.hpp"

#include "consumer/consumer.hpp"

Consumer::Consumer(asio::io_context& io, const std::string& host,
                   unsigned short port_base, int min_ms, int max_ms)
    : BaseClient(io, host, port_base),
      timer_(io),
      min_ms_(min_ms),
      max_ms_(max_ms) {}

void Consumer::OnReady() { asio::co_spawn(io_, TickLoop(), asio::detached); }

// 호가창 수신 시 mid 갱신
void Consumer::OnOrderBook(const Orderbook& book) {
  const bool has_bid = book.bid_qty[0] > 0;
  const bool has_ask = book.ask_qty[0] > 0;
  if (has_bid && has_ask)
    mid_ = (book.bid_price[0] + book.ask_price[0]) / 2.0;
  else if (has_bid)
    mid_ = book.bid_price[0];
  else if (has_ask)
    mid_ = book.ask_price[0];
}

// 주기마다 매수/매도를 고른다.
//  - side : mid 가 기준가보다 높으면 매도확률↑ -> 기준가로 평균회귀
//  - 가격 : 현재 mid 부근 랜덤.
asio::awaitable<void> Consumer::TickLoop() {
  std::uniform_int_distribution<int> delay(min_ms_, max_ms_);
  std::uniform_int_distribution<int> offset(-kPriceRange, kPriceRange);
  boost::system::error_code ec;
  for (;;) {
    // 기준가보다 현재 시장 mid가 높으면 sell확률 증가
    const double dev = mid_ - kSeedMid;
    const double p_sell = std::clamp(0.5 + kMeanReversionBias * dev, 0.0, 1.0);
    const Side side =
        std::bernoulli_distribution(p_sell)(rng_) ? Side::kSell : Side::kBuy;
    const int center = static_cast<int>(mid_);  // 목표가는 현재 시장 기준
    Reconcile(side, std::max(1, center + offset(rng_)));

    timer_.expires_after(std::chrono::milliseconds(delay(rng_)));
    co_await timer_.async_wait(asio::redirect_error(asio::use_awaitable, ec));
    if (ec) co_return;  // Stop() 으로 타이머 취소됨
  }
}

void Consumer::OnStop() { timer_.cancel(); }

// 신규 주문마다 1~kMaxQty 랜덤 수량 (client 는 kOrderQty 고정).
int Consumer::OrderQty() {
  std::uniform_int_distribution<int> qty(1, kMaxQty);
  return qty(rng_);
}
