// 서버 클라이언트가 공유하는 통신 프로토콜 관련 내용
// orderbook 프로토콜 - 호가창 정보
// 주문 프로토콜 - 주문/주문결과
#pragma once

#include "pch.hpp"

// ---------------------------------------------------------------------------
// orderbook 프로토콜
// ---------------------------------------------------------------------------
// 과제문서 order book format 참고
// 네트워크/호가창 관련 상수는 common/definitions.hpp 참고

// 필드 단위
using TickerField = std::array<uint8_t, kTickerLen>;
using PriceField = std::array<uint8_t, kPriceLen>;
using QtyField = std::array<uint8_t, kQtyLen>;
// 5단계 가격 & 수량
using PriceLayers = std::array<PriceField, kNumLayers>;
using QtyLayers = std::array<QtyField, kNumLayers>;

struct OrderBookPacket {
  TickerField ticker{};     // 0 ~ 3 (4bytes)
  PriceLayers bid_price{};  // 4 ~ 28 (5 * 5 = 25bytes)
  PriceLayers ask_price{};  // 29 ~ 53 (5 * 5 = 25bytes)
  QtyLayers bid_qty{};      // 54 ~ 73 (4 * 5 = 20bytes)
  QtyLayers ask_qty{};      // 74 ~ 93 (4 * 5 = 20bytes)
};
static_assert(sizeof(OrderBookPacket) == kOrderBookSize,
              "OrderBookPacket must be a packed 94-byte record");

// ---------------------------------------------------------------------------
// 주문 프로토콜
// ---------------------------------------------------------------------------
//   주문전송
//   side  : 매수/매도
//   price : 주문가격
//   qty   : 주문량
//   qty == 0 : 해당 'side'/'price'의 주문 취소
//
// server -> client (OrderResultPacket):
//   체결(Sucess) : 'side'/'price'에서 'qty'만큼 체결됨
//   거부(reject): 이미 체결됐거나 존재하지 않는 주문에 대한 취소 요청

enum class Side : int32_t { kBuy = 0, kSell = 1 };

struct OrderPacket {
  Side side = Side::kBuy;
  int32_t price = 0;
  int32_t qty = 0;
  uint32_t seq = 0;  // 주문 일련번호 (주문처리 latency 측정목적)
};
static_assert(sizeof(OrderPacket) == 16,
              "OrderPacket must be a packed 16-byte record");

enum class ResultType : int32_t {
  kSuccess = 0,    // 체결
  kReject = 1,     // 취소 거부 (이미 체결/미존재)
  kCancelled = 2,  // 취소 완료
  kAccepted = 3    // 신규주문 등록 완료
};

struct OrderResultPacket {
  ResultType type = ResultType::kSuccess;
  Side side = Side::kBuy;
  int32_t price = 0;
  int32_t qty = 0;   // fill: 체결수량 / reject: 거부된 주문 수량
  uint32_t seq = 0;  // 주문 일련번호  (주문처리 latency 측정목적)
};
static_assert(sizeof(OrderResultPacket) == 20,
              "ExecPacket must be a packed 20-byte record");