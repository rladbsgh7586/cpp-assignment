// 서버 진입점
// 기초 연결 예제
// - 키보드 'q' 로 종료 (Enter 불필요)
#include "pch.hpp"

#include "server/server.hpp"

int main() {
  logger::InitFileLog("server");
  asio::io_context io;
  Server server(io, kPortBase);
  KeyboardInput quit(io, 'q', [&] {
    LOG_INFO("q 입력 - 서버 종료");
    server.stop();
    io.stop();
  });

  LOG_INFO("server listening on port {} (종료: q)", kPortBase);

  io.run();
  return 0;
}
