// 주문(OE) + 시장데이터(MD) 양 채널에 접속하는 거래 클라이언트 기반 클래스
// 파생 클래스는 주문 정책만 결정한다.
//  client: OnOrderBook() override하여 호가창 수신시점에 주문
//  consumer: 타이머 트리거
#pragma once

#include "pch.hpp"

class BaseClient {
 public:
  // measure_latency=true 이면 주문 round-trip latency 를 측정/기록한다.
  BaseClient(asio::io_context& io, const std::string& host, uint16_t port_base,
             bool measure_latency = false);
  virtual ~BaseClient() = default;

  void Stop();  // 양 채널 종료 (+ 파생 정리 훅 OnStop)

 protected:
  virtual void OnOrderBook(const Orderbook& book) { (void)book; }
  virtual void OnReady() {}  // 타이머 루프 시작 등에 사용.
  virtual void OnAssignedId(int id) { logger::SetInstanceId(id); }
  virtual void OnStop() {}
  // 신규 주문 Qunatity. client는 고정값 (10). consumer는 랜덤
  virtual int OrderQty() { return kOrderQty; }

  // side 당 주문 1개를 유지하며 목표가로 맞춘다 (취소 후 재호가).
  void Reconcile(Side side, int desired);

  asio::io_context& io_;

 private:
  asio::awaitable<void> Run(std::string host, uint16_t port_base);
  asio::awaitable<void> ConnectOrderEntry(const std::string& host,
                                          uint16_t port);
  asio::awaitable<void> OeReader();
  asio::awaitable<void> ConnectMarketData(const std::string& host,
                                          uint16_t port);
  asio::awaitable<void> MdReader();
  void SendOrder(Side side, int price, int qty);

  // 한 side 주문의 OE 상태머신. 모든 전이는 OeReader 가 서버 결과를 받아 수행.
  enum class OeState {
    kIdle,          // 주문 없음
    kWaitAccepted,  // 신규주문 전송 -> kAccepted 대기
    kResting,       // 주문 완료 상태 (kOrderQty만큼 주문되어있는 상태)
    kWaitCanceled   // 취소주문 전송 -> kCancelled 대기
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

  tcp::socket md_socket_;
  tcp::socket oe_socket_;
  tcp::resolver resolver_;
  bool oe_ready_ = false;

  // latency 측정
  struct SendHistory {
    std::chrono::steady_clock::time_point sent;
    int64_t send_epoch_ns;
    Side side;
    int price;
  };
  bool latency_enabled_ = false;
  uint32_t next_seq_ = 1;
  std::unordered_map<uint32_t, SendHistory> inflight_;
  std::optional<LatencyLog> latency_log_;
};
