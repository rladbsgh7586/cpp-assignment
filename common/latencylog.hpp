// 주문 latency 기록기
//   - debug 모드에서만 생성/사용
//   - 포맷:  <send_epoch_ns> <BID|ASK> <price> <latency_ns>
#pragma once

#include "pch.hpp"

class LatencyLog {
 public:
  // log/latency_<id>.txt 를 append 모드로 open
  explicit LatencyLog(int id);

  // 주문 1건의 측정 결과를 한 줄 기록.
  //   send_epoch_ns : 송신 시점의 system_clock epoch nanoseconds
  //   side          : 주문 방향
  //   price         : 주문 가격
  //   latency_ns    : 송신 -> 응답(ACK) round-trip nanoseconds
  void Record(int64_t send_epoch_ns, Side side, int price, int64_t latency_ns);

 private:
  std::ofstream file_;
};
