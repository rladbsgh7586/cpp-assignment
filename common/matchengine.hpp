// 실제 주문이 등록되는 매칭엔진
//
// 각 호가에는 선입선출(FIFO)로 client 주문이 등록
// 정의된 5단계가 아니라 실제로는 더 넓게 관리
// 매수/매도호가에서 5단계씩 추출하여 OrderBookPacket으로 전송
#pragma once

#include "pch.hpp"

// OrderBookPacket을 디코드한 정수 호가창 뷰 (최우선호가부터 정렬).
struct Orderbook {
  std::array<int, kNumLayers> bid_price{};
  std::array<int, kNumLayers> bid_qty{};
  std::array<int, kNumLayers> ask_price{};
  std::array<int, kNumLayers> ask_qty{};
};

// 주문 처리 결과 - 해당 클라이언트에게만 결과전송
struct OrderResult {
  int client_id;
  ResultType type;
  Side side;
  int price;
  int qty;
  uint32_t seq;  // 주문 일련번호 (주문처리 latency 측정목적)
};

class MatchingEngine {
 public:
  // 주문처리
  // marketdata 구조가 바뀌면 dirty 가 true 로 설정된다. (order reject같은경우 dirty false)
  // OrderResult는 체결순서대로 반환 (1개 주문 체결시 결과가 여러개 나옴)
  std::vector<OrderResult> PushOrder(int client_id, const OrderPacket& order,
                                     bool& dirty);

  // 최우선 5단계를 동일가 수량 합산
  OrderBookPacket ExtractOrderBook() const;

 private:
  struct RestingOrder {
    int client_id;
    int qty;       // 잔량 (FIFO 순서는 deque 삽입 순서로 보장)
    uint32_t seq;  // 주문 일련번호 (주문처리 latency 측정목적)
  };
  // price, order queue, 정렬 순서
  // BidMap: 매수니까 내림차순 예) 100(최우선 매수호가), 99, 98, ..
  // AskMap: 매도니까 오름차순 예) 105(최우선 매도호가), 106, 107, ..
  //
  // map, deque 선택 이유
  // 1. match시 최우선 호가부터 처리하므로 정렬되어있어야함.
  // 2. client의 주문 처리를 위해 중간 접근 & 삭제가 자유로워야함.
  // Match에 begin -> O(n)
  // Cancel, Rest등에 find -> O(log n)
  using BidMap = std::map<int, std::deque<RestingOrder>, std::greater<int>>;
  using AskMap = std::map<int, std::deque<RestingOrder>, std::less<int>>;

  // 각 함수는 PushOrder시 주문 처리 과정에서 사용됨
  // dirty = false -> 주문 취소로 인해 orderbook 변경없음 -> broadcast 필요없음
  // 주문 체결
  template <typename OppMap>
  void Match(int client_id, uint32_t seq, Side side, int price, int& qty,
             OppMap& opp, std::vector<OrderResult>& out, bool& dirty);
  // 주문 등록
  void Rest(int client_id, uint32_t seq, Side side, int price, int qty,
            std::vector<OrderResult>& out);
  // 주문 취소
  void Cancel(int client_id, uint32_t seq, Side side, int price,
              std::vector<OrderResult>& out, bool& dirty);

  BidMap bids_;  // 가격 내림차순
  AskMap asks_;  // 가격 오름차순
};

// 정수 호가창 뷰 공백 필드는 0.
Orderbook Parse(const OrderBookPacket& packet);
