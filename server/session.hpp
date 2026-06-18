// 서버 세션
// shared_ptr 로 라이프사이클 관리
#pragma once

#include "pch.hpp"

// 시장데이터 client (server -> client 단방향 broadcast).
// 느린 구독자를 위해 최신 스냅샷 1개만 보관(coalescing)하여 큐 폭주를 막는다.
struct MdSession {
  tcp::socket socket;
  int id;
  OrderBookPacket pending{};
  bool has_pending = false;
  bool writing = false;

  MdSession(tcp::socket s, int i) : socket(std::move(s)), id(i) {}
};

// 주문 채널 세션 (client <-> server 양방향)
struct OeSession {
  tcp::socket socket;
  int client_id;
  std::deque<OrderResultPacket> wq;
  bool writing = false;

  OeSession(tcp::socket s, int i) : socket(std::move(s)), client_id(i) {}
};
