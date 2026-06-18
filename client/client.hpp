// 시장추종 전략 클라이언트
//  - orderbook을 수신하여 Q/N 인자에 맞게 최우선 매수/매도호가 주문
#pragma once

#include "pch.hpp"

class Client {
 public:
  Client(asio::io_context& io, const std::string& host, uint16_t port_base,
         int q, int n);
  void Stop();

 private:
  asio::awaitable<void> Run(std::string host, uint16_t port_base);

  asio::awaitable<void> ConnectOrderEntry(const std::string& host,
                                            uint16_t port);
  asio::awaitable<void> OeReader();

  asio::awaitable<void> ConnectMarketData(const std::string& host,
                                            uint16_t port);
  asio::awaitable<void> MdReader();

  void Reconcile(Side side, int desired);  // cancel-before-replace
  void SendOrder(Side side, int price, int qty);

  // 한 side 주문의 OE 상태머신. 모든 전이는 OeReader 가 서버 결과를 받아 수행.
  enum class OeState {
    kIdle,          // 주문 없음
    kWaitAccepted,  // 신규주문 전송 -> kAccepted 대기
    kResting,       // 주문 완료 상태 (kOrderQty만큼 주문되어있는 상태)
    kWaitCanceled   // 취소주문 전송 -> kCan
  };
  struct SideState {
    OeState state = OeState::kIdle;
    int price = 0;    // 현재 주문 가격
    int qty = 0;      // 잔량
    int pending = 0;  // 새 목표가
  };

  SideState& StateOf(Side side) { return side == Side::kBuy ? bid_ : ask_; }

  SideState bid_;
  SideState ask_;

  asio::io_context& io_;
  tcp::socket md_socket_;
  tcp::socket oe_socket_;
  tcp::resolver resolver_;
  int q_;
  int n_;
  bool oe_ready_ = false;
};
