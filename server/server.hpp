// TCP 서버
//  시장데이터 채널 (9000)
//    - 모든 클라이언트에게 94bytes orderbook broadcast
//    - orderbook은 클라이언트 최초연결 또는 주문이 체결될때마다 발행됨
//  주문 채널 (9001)
//    client 주문(OrderPacket) 수신 -> 매칭엔진 -> 해당 client 에 주문결과 보고
#pragma once

#include "pch.hpp"

#include "server/session.hpp"

class Server {
 public:
  Server(asio::io_context& io, unsigned short port_base);
  void Stop();

 private:
  asio::awaitable<void> MdListener();
  asio::awaitable<void> MdWriter(std::shared_ptr<MdSession> s);
  asio::awaitable<void> MdWatch(std::shared_ptr<MdSession> s);
  asio::awaitable<void> OeListener();
  asio::awaitable<void> OeReader(std::shared_ptr<OeSession> s);
  asio::awaitable<void> OeWriter(std::shared_ptr<OeSession> s);

  // 매칭 결과를 각 주문 client 에게 전송 / 전체 구독자에 스냅샷 발행.
  void SendOrderResults(const std::vector<OrderResult>& results);
  void BroadcastOrderBook();

  // 서버 시작 시 초기 호가창 주문 삽입
  // price: 95 ~ 105
  // qty: 50 (for each)
  void InitBook();

  asio::io_context& io_;
  tcp::acceptor md_acceptor_;
  tcp::acceptor oe_acceptor_;
  MatchingEngine engine_;
  std::unordered_map<int, std::shared_ptr<MdSession>> md_sessions_;
  std::unordered_map<int, std::shared_ptr<OeSession>> oe_sessions_;
  int next_md_id_ = 1;
  int next_oe_id_ = 1;
};
