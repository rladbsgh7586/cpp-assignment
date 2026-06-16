// 서버 클라이언트가 공유하는 통신 프로토콜 관련 내용
// orderbook 프로토콜 - 호가창 정보
// 주문 프로토콜 - 주문/주문결과
#pragma once

#include "pch.hpp"

// ---------------------------------------------------------------------------
// 네트워크
// ---------------------------------------------------------------------------
inline constexpr char kServerHost[] = "127.0.0.1";  // client 접속 대상
inline constexpr unsigned short kPortBase = 9000;   // 기본 포트
inline constexpr int kMarketDataPortOffset = 0;     // PORT_BASE + 0
inline constexpr int kOrderEntryPortOffset = 1;     // PORT_BASE + 1

// ---------------------------------------------------------------------------
// orderbook 프로토콜
// ---------------------------------------------------------------------------
// 과제문서 order book format 참고

inline constexpr int kNumLayers = 5;  // 호가 단계 수
inline constexpr int kTickerLen = 4;  // Ticker string bytes
inline constexpr int kPriceLen = 5;   // Price string bytes
inline constexpr int kQtyLen = 4;     // Quantity string bytes

inline constexpr int kOrderBookSize =
    kTickerLen + 2 * kNumLayers * kPriceLen + 2 * kNumLayers * kQtyLen;  // 94

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
// client -> server:
//   주문전송
//   Side : 매수/매도
//   price : 주문가격
//   qty : 주문량
//   qty == 0 : 해당 'side'/'price'의 주문 취소
//
// server -> client:
//   체결결과
//   주문이 'side'/'price'에서 'qty'만큼 체결됨.

enum class Side : int32_t { kBuy = 0, KSell = 1 };

struct OrderPacket {
  Side side = Side::kBuy;
  int32_t price = 0;
  int32_t qty = 0;
};
static_assert(sizeof(OrderPacket) == 12,
              "OrderPacket must be a packed 12-byte record");