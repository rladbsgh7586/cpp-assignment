// 클라이언트 진입점
// Q, N을 기반으로 OrderBook을 수신할때마다 최적의 매수/매도
// usage: ./client <Q> <N>
//  Q : 최우선 매수/매도호가를 식별하는 누적 수량 (>= 0)
//  N : 최우선호가에서 떨어진 틱 수 (>= 0)
//
// 연결포트
// 9000: MarketDataPort (Orderbook 수신)
// 9001: OrderEntryPort (주문송신/결과수신)
//
// - 키보드 'q' 로 종료
#include "pch.hpp"

#include "client/client.hpp"

int main(int argc, char** argv) {
  // arg parse
  std::vector<std::string_view> pos;
  for (int i = 1; i < argc; ++i) {
    std::string_view a = argv[i];
    if (a == "-d" || a == "--debug")
      logger::SetLogLevel(logger::LogLv::kDebug);
    else
      pos.push_back(a);
  }

  int q = 0, n = 0;
  if (pos.size() != 2 || !ParseNonNeg(pos[0], q) || !ParseNonNeg(pos[1], n)) {
    std::cout << "usage: ./client <Q> <N> [-d|--debug]" << std::endl;
    return 1;
  }

  logger::InitFileLog("client");

  asio::io_context io;
  Client client(io, kServerHost, kPortBase, q, n);
  KeyboardInput quit(io, 'q', [&] {
    LOG_INFO("q 입력 - 클라이언트 종료");
    client.Stop();
    io.stop();
  });

  LOG_INFO("client 연결 Q={} N={} -> {}:{} (종료: q)", q, n, kServerHost,
           kPortBase);

  io.run();
  return 0;
}
