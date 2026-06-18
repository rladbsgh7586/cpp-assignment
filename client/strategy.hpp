// 제출할 최우선 매수/매도호가 가격 로직
#pragma once

#include "pch.hpp"

struct BestPrice {
  int bid_price;  // 제출할 best_bid
  int ask_price;  // 제출할 best_price
};

namespace strategy {

// price는 우선순위에 맞게 정렬된 상태
inline std::optional<int> BestSidePrice(
    const std::array<int, kNumLayers>& price,
    const std::array<int, kNumLayers>& qty, int q) {
  if (qty[0] <= 0) return std::nullopt;  // 시장 호가 없음
  if (q <= 0) return price[0];           // 최우선호가

  int sum = 0;
  for (int i = 0; i < kNumLayers; ++i) {
    if (qty[i] <= 0) break;
    sum += qty[i];
    if (sum >= q) return price[i];
  }

  // 시장 유동성이 충분하지 않은 케이스 -> 최우선호가를 지정하지 않음
  return std::nullopt;
}

// 양쪽 호가가 모두 존재할 때만 목표 최우선호가 반환
inline std::optional<BestPrice> CalcBestPrice(const Orderbook& book, int q,
                                              int n) {
  auto best_bid = BestSidePrice(book.bid_price, book.bid_qty, q);
  auto best_ask = BestSidePrice(book.ask_price, book.ask_qty, q);
  if (!best_bid || !best_ask) return std::nullopt;
  return BestPrice{*best_bid - n, *best_ask + n};
}

}  // namespace strategy
