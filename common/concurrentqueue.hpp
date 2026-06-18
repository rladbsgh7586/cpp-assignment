// 스레드 간 단방향 전달용 큐.
//  - pub: io_context 스레드 (통신모듈).
//  - sub: 매칭엔진 스레드
#pragma once

#include "pch.hpp"

template <typename T>
class ConcurrentQueue {
 public:
  void Push(T value) {
    {
      std::lock_guard<std::mutex> lk(mtx_);
      q_.push_back(std::move(value));
    }
    cv_.notify_one();
  }

  bool WaitPop(std::stop_token st, T& out) {
    std::unique_lock<std::mutex> lk(mtx_);
    if (!cv_.wait(lk, st, [this] { return !q_.empty(); })) {
      return false;  // stop 요청 & 큐 비어있음
    }
    out = std::move(q_.front());
    q_.pop_front();
    return true;
  }

 private:
  mutable std::mutex mtx_;
  std::condition_variable_any cv_;
  std::deque<T> q_;
};
