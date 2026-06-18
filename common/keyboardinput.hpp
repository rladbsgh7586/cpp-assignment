// 콘솔 표준입력(stdin)을 비동기로 감시
// 지정한 키 입력시 콜백 호출
// server, client의 "q" 입력 종료에 사용
#pragma once

#include "pch.hpp"

class KeyboardInput {
 public:
  KeyboardInput(asio::io_context& io, char key, std::function<void()> callback)
      : stdin_(io, dup(STDIN_FILENO)),
        key_(key),
        callback_(std::move(callback)) {
    EnableRawMode();
    asio::co_spawn(stdin_.get_executor(), Watch(), asio::detached);
  }

  ~KeyboardInput() { RestoreMode(); }

  // rule of five
  KeyboardInput(const KeyboardInput&) = delete;
  KeyboardInput& operator=(const KeyboardInput&) = delete;
  KeyboardInput(KeyboardInput&&) = delete;
  KeyboardInput& operator=(KeyboardInput&&) = delete;

 private:
  // canonical(줄 단위) + echo 를 꺼서 키 하나가 즉시 들어오게 한다.
  void EnableRawMode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios_) != 0) {
      return;
    }
    raw_active_ = true;
    termios raw = orig_termios_;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  }

  void RestoreMode() {
    if (raw_active_) {
      tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios_);
      raw_active_ = false;
    }
  }

  // target key가 도착하면 callback 호출
  asio::awaitable<void> Watch() {
    try {
      char ch = 0;
      for (;;) {
        co_await asio::async_read(stdin_, asio::buffer(&ch, 1),
                                  asio::use_awaitable);
        if (ch == key_) {
          callback_();
          co_return;
        }
      }
    } catch (const std::exception&) {
      // 에러 케이스 -> io.run 종료
    }
  }

  asio::posix::stream_descriptor stdin_;
  char key_;
  std::function<void()> callback_;
  termios orig_termios_{};
  bool raw_active_ = false;
};
