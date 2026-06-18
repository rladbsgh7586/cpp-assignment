// 시장추종 전략 클라이언트
//  - orderbook을 수신하여 Q/N 인자에 맞게 최우선 매수/매도호가 주문
//  - 연결/주문 라이프사이클은 BaseClient 공유. 목표가 계산만 여기서 담당.
#pragma once

#include "pch.hpp"

class Client : public BaseClient {
 public:
  Client(asio::io_context& io, const std::string& host, uint16_t port_base,
         int q, int n);

 private:
  // 호가창 수신 시 Q/N 기반 목표 최우선호가를 계산해 양 side 재조정.
  void OnOrderBook(const Orderbook& book) override;

  int q_;
  int n_;
};
