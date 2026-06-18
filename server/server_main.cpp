// 서버 진입점
// - 시장데이터 채널: kPortBase + 0 (orderbook broadcast)
// - 주문 채널     : kPortBase + 1 (주문 수신 / 주문처리결과 송신)
// - 키보드 'q' 로 종료 (Enter 불필요)
#include "pch.hpp"

#include "server/server.hpp"

int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    std::string_view a = argv[i];
    if (a == "-d" || a == "--debug") logger::SetLogLevel(logger::LogLv::kDebug);
  }

  logger::InitFileLog("server");
  asio::io_context io;
  Server server(io, kPortBase);
  KeyboardInput quit(io, 'q', [&] {
    LOG_INFO("q 입력 - 서버 종료");
    server.Stop();
    io.stop();
  });

  LOG_INFO("server listening: md={}, oe={} (종료: q)",
           kPortBase + kMarketDataPortOffset,
           kPortBase + kOrderEntryPortOffset);

  io.run();
  return 0;
}
