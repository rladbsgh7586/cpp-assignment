#include "pch.hpp"

#include "client/client.hpp"

#include "client/strategy.hpp"

Client::Client(asio::io_context& io, const std::string& host,
               uint16_t port_base, int q, int n, bool debug)
    : BaseClient(io, host, port_base, debug), q_(q), n_(n) {}

void Client::OnOrderBook(const Orderbook& book) {
  // Q/N에 기반하여 최우선 매수호가/매도호가 계산
  auto best = strategy::CalcBestPrice(book, q_, n_);
  // best가 없는경우 -> 유동성 부족으로 양쪽 최우선호가가 충족되지 않을때
  if (best) {
    Reconcile(Side::kBuy, best->bid_price);
    Reconcile(Side::kSell, best->ask_price);
  }
}
