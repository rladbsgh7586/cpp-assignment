// 일반 소비자 클라이언트
//  - 타이머 주기 (min_ms, max_ms)마다 랜덤한 주문 전송
//  - 랜덤한 가격 (kSeedMid ± kPriceRange)
//  - 랜덤한 주문 수량
//  - 호가창의 mid 를 보고 매수/매도 선택 확률 변동 -> 기준가로 수렴하게함
#pragma once

#include "pch.hpp"

class Consumer : public BaseClient {
 public:
  // 주문 주기는 [min_ms, max_ms] 범위에서 매 틱 랜덤 추출 (빈도 인자는 main 이 매핑).
  Consumer(asio::io_context& io, const std::string& host,
           unsigned short port_base, int min_ms, int max_ms);

 private:
  void OnReady() override;            // 연결 완료 후 타이머 루프 시작
  void OnAssignedId(int) override {}  // 로그 태그는 main 이 정한 band id 유지
  void OnStop() override;             // 타이머 취소
  int OrderQty() override;            // client 와 달리 수량을 랜덤화 (노이즈)
  void OnOrderBook(const Orderbook& book) override;  // mid 추적

  asio::awaitable<void> TickLoop();  // 주기마다 (편향)랜덤 side/목표가로 재조정

  asio::steady_timer timer_;
  std::mt19937 rng_{std::random_device{}()};
  double mid_ = kSeedMid;  // 최근 호가창 mid
  int min_ms_;
  int max_ms_;
};
