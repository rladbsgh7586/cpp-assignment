#include "pch.hpp"

#include "consumer/consumer.hpp"

Consumer::Consumer(asio::io_context& io, const std::string& host,
                   unsigned short port_base, int min_ms, int max_ms)
    : BaseClient(io, host, port_base),
      timer_(io),
      min_ms_(min_ms),
      max_ms_(max_ms) {}

void Consumer::OnReady() { asio::co_spawn(io_, TickLoop(), asio::detached); }

// 주기마다 매수/매도 한쪽을 랜덤으로 골라 seed mid 부근 랜덤 목표가로 Reconcile.
asio::awaitable<void> Consumer::TickLoop() {
  std::uniform_int_distribution<int> delay(min_ms_, max_ms_);
  std::uniform_int_distribution<int> offset(-kPriceRange, kPriceRange);
  std::bernoulli_distribution buy(0.5);
  boost::system::error_code ec;
  for (;;) {
    const Side side = buy(rng_) ? Side::kBuy : Side::kSell;
    Reconcile(side, std::max(1, kSeedMid + offset(rng_)));

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
