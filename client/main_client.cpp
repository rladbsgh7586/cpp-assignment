// 클라이언트 진입점 (기초 예제)
//   usage: ./client   (접속 대상은 protocol.hpp 의 kServerHost:kPortBase 고정)
//   - 서버가 1초마다 보내는 elapsed 메시지를 출력
//   - 키보드 'q' 로 종료 (Enter 불필요)
#include "pch.hpp"

#include "client/client.hpp"

int main() {
  logger::InitFileLog("client");  // 콘솔 + log/client.log 동시 출력
  asio::io_context io;
  Client client(io, kServerHost, kPortBase);
  KeyboardInput quit(io, 'q', [&] {
    LOG_INFO("q 입력 - 클라이언트 종료");
    client.stop();
    io.stop();
  });

  LOG_INFO("connecting to {}:{} (종료: q)", kServerHost, kPortBase);

  io.run();
  return 0;
}
