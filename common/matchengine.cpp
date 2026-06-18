#include "pch.hpp"

namespace {

// 정수를 좌측정렬 텍스트로 변환
void PutField(uint8_t* dst, int width, int value, bool empty) {
  std::memset(dst, ' ', width);
  if (empty || value <= 0) return;
  char tmp[16];
  auto res = std::to_chars(tmp, tmp + sizeof(tmp), value);
  int n = static_cast<int>(res.ptr - tmp);
  if (n > width) {
    LOG_WARN("PutField overflow: value={} width={}", value, width);
    return;
  }
  std::memcpy(dst, tmp, static_cast<size_t>(n));
}

// 좌측정렬 텍스트를 정수로 파싱 (공백뿐이면 0)
int GetField(const uint8_t* src, int width) {
  int value = 0;
  std::from_chars(reinterpret_cast<const char*>(src),
                  reinterpret_cast<const char*>(src) + width, value);
  return value;
}

}  // namespace

std::vector<OrderResult> MatchingEngine::PushOrder(int client_id,
                                                   const OrderPacket& order,
                                                   bool& dirty) {
  std::vector<OrderResult> out;
  dirty = false;

  if (order.qty == 0) {  // 주문취소
    Cancel(client_id, order.seq, order.side, order.price, out, dirty);
    return out;
  }
  if (order.qty < 0 || order.price <= 0) {  // 잘못된 주문
    out.push_back({client_id, ResultType::kReject, order.side, order.price, 0,
                   order.seq});
    LOG_WARN("주문 거부: 잘못된 price/qty (client #{} {} {}@{})", client_id,
             SideStr(order.side), order.qty, order.price);
    return out;
  }

  int qty = order.qty;
  // 주문 체결
  if (order.side == Side::kBuy) {
    Match(client_id, order.seq, order.side, order.price, qty, asks_, out,
          dirty);
  } else {
    Match(client_id, order.seq, order.side, order.price, qty, bids_, out,
          dirty);
  }

  if (qty > 0) {
    // 체결되지 않은 주문은 orderbook 에 등록
    Rest(client_id, order.seq, order.side, order.price, qty, out);
    dirty = true;
  }
  return out;
}

template <typename OppMap>
// 주문 체결 - price까지 qty만큼 반대편 호가를 체결 진행
// 참고 AskMap = std::map<int, std::deque<RestingOrder>, std::greater<int>>;
void MatchingEngine::Match(int client_id, uint32_t seq, Side side, int price,
                           int& qty, OppMap& opp, std::vector<OrderResult>& out,
                           bool& dirty) {
  const bool is_buy = side == Side::kBuy;
  const Side opp_side = is_buy ? Side::kSell : Side::kBuy;

  // 주문 순차 체결
  while (qty > 0 && !opp.empty()) {
    auto it = opp.begin();  // 반대편 최우선 호가
    const int rest_price = it->first;
    const bool is_cross =
        is_buy ? (rest_price <= price) : (rest_price >= price);
    if (!is_cross) break;

    auto& fifo = it->second;  // 동일가 FIFO (front = 가장 오래된 주문)
    while (qty > 0 && !fifo.empty()) {
      RestingOrder& rest_order = fifo.front();
      const int matched_qty = std::min(qty, rest_order.qty);
      // 체결가는 수동(resting) 주문의 가격.
      qty -= matched_qty;
      rest_order.qty -= matched_qty;
      dirty = true;

      out.push_back({client_id, ResultType::kSuccess, side, rest_price,
                     matched_qty, seq});
      out.push_back({rest_order.client_id, ResultType::kSuccess, opp_side,
                     rest_price, matched_qty, rest_order.seq});
      LOG_INFO("체결 {}@{} taker=#{} maker=#{}", matched_qty, rest_price,
               client_id, rest_order.client_id);

      if (rest_order.qty == 0) fifo.pop_front();
    }
    // 해당 호가의 RestOrder 전부 소진
    if (fifo.empty()) opp.erase(it);
  }
}

// 주문 등록
void MatchingEngine::Rest(int client_id, uint32_t seq, Side side, int price,
                          int qty, std::vector<OrderResult>& out) {
  RestingOrder ro{client_id, qty, seq};
  if (side == Side::kBuy)
    bids_[price].push_back(ro);
  else
    asks_[price].push_back(ro);
  out.push_back({client_id, ResultType::kAccepted, side, price, qty, seq});
  LOG_INFO("등록 #{} {} {}@{}", client_id, SideStr(side), qty, price);
}

// 주문 취소
void MatchingEngine::Cancel(int client_id, uint32_t seq, Side side, int price,
                            std::vector<OrderResult>& out, bool& dirty) {
  // 주문 성격 (Bid, Ask)에 맞게 알맞은 Map에서 search + erase
  auto remove_from = [&](auto& book) -> bool {
    auto lit = book.find(price);
    if (lit == book.end()) return false;
    auto& fifo = lit->second;
    for (auto qit = fifo.begin(); qit != fifo.end(); ++qit) {
      if (qit->client_id == client_id) {
        fifo.erase(qit);
        if (fifo.empty()) book.erase(lit);
        return true;
      }
    }
    return false;
  };

  const bool ok = side == Side::kBuy ? remove_from(bids_) : remove_from(asks_);
  if (ok) {
    dirty = true;
    out.push_back({client_id, ResultType::kCancelled, side, price, 0, seq});
    LOG_INFO("취소 #{} {} @{}", client_id, SideStr(side), price);
    return;
  }

  // 취소 실패. 주문 미존재 (이미 체결됐거나 다른 이유)
  out.push_back({client_id, ResultType::kReject, side, price, 0, seq});
  LOG_INFO("취소 거부 #{} {} @{} (주문 미존재)", client_id, SideStr(side),
           price);
}

// 현재 실제 시장데이터에서 5layer 데이터만 extract해서 orderbook 생성
OrderBookPacket MatchingEngine::ExtractOrderBook() const {
  OrderBookPacket pkt;
  auto* raw = reinterpret_cast<uint8_t*>(&pkt);
  std::memset(raw, ' ', sizeof(pkt));
  std::memcpy(pkt.ticker.data(), "6800", 3);

  int i = 0;
  for (auto it = bids_.begin(); it != bids_.end() && i < kNumLayers;
       ++it, ++i) {
    int sum = 0;
    for (const auto& o : it->second) sum += o.qty;
    PutField(pkt.bid_price[i].data(), kPriceLen, it->first, false);
    PutField(pkt.bid_qty[i].data(), kQtyLen, sum, false);
  }
  i = 0;
  for (auto it = asks_.begin(); it != asks_.end() && i < kNumLayers;
       ++it, ++i) {
    int sum = 0;
    for (const auto& o : it->second) sum += o.qty;
    PutField(pkt.ask_price[i].data(), kPriceLen, it->first, false);
    PutField(pkt.ask_qty[i].data(), kQtyLen, sum, false);
  }
  return pkt;
}

Orderbook Parse(const OrderBookPacket& packet) {
  Orderbook v;
  for (int i = 0; i < kNumLayers; ++i) {
    v.bid_price[i] = GetField(packet.bid_price[i].data(), kPriceLen);
    v.bid_qty[i] = GetField(packet.bid_qty[i].data(), kQtyLen);
    v.ask_price[i] = GetField(packet.ask_price[i].data(), kPriceLen);
    v.ask_qty[i] = GetField(packet.ask_qty[i].data(), kQtyLen);
  }
  return v;
}
