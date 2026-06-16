// TCP 세션
// Session은 shared_ptr로 라이프사이클 관리
#pragma once

#include "pch.hpp"

struct Session {
  tcp::socket socket;
  int id;

  Session(tcp::socket s, int i) : socket(std::move(s)), id(i) {}
};
