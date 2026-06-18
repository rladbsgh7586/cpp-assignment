#pragma once

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;

// ---------------------------------------------------------------------------
// 네트워크
// ---------------------------------------------------------------------------
inline constexpr char kServerHost[] = "127.0.0.1";  // client 접속 대상
inline constexpr unsigned short kPortBase = 9000;   // 기본 포트
inline constexpr int kMarketDataPortOffset = 0;     // PORT_BASE + 0
inline constexpr int kOrderEntryPortOffset = 1;     // PORT_BASE + 1

// ---------------------------------------------------------------------------
// 호가창
// ---------------------------------------------------------------------------
inline constexpr int kNumLayers = 5;  // 호가 단계 수
inline constexpr int kTickerLen = 4;  // Ticker string bytes
inline constexpr int kPriceLen = 5;   // Price string bytes
inline constexpr int kQtyLen = 4;     // Quantity string bytes

inline constexpr int kOrderBookSize =
    kTickerLen + 2 * kNumLayers * kPriceLen + 2 * kNumLayers * kQtyLen;  // 94

// ---------------------------------------------------------------------------
// server seed (초기 호가창)
// ---------------------------------------------------------------------------
inline constexpr int kSeedMid = 100;       // seed 호가창 중심가
inline constexpr int kSeedQty = 50;        // seed 주문 수량
inline constexpr int kSeedClientId = -10;  // 가상 client

// ---------------------------------------------------------------------------
// client
// ---------------------------------------------------------------------------
inline constexpr int kOrderQty = 10;  // 주문 수량

// ---------------------------------------------------------------------------
// consumer
// ---------------------------------------------------------------------------
inline constexpr int kPriceRange = 15;  // 목표가 = seed mid ± 이 범위 랜덤
inline constexpr int kMaxQty = 20;      // 주문 수량 랜덤 상한 (1~kMaxQty)
