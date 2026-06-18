// 일반 소비자(노이즈 트레이더) 진입점
//   usage: ./consumer <high|mid|low> [id] [-d|--debug]
//     high/mid/low: 주문 빈도
//     id   : 로그 태그 구분용
//   - SIGINT/SIGTERM 으로 종료 (샘플 스크립트가 일괄 kill)
#include "pch.hpp"

#include "consumer/consumer.hpp"

namespace {
// 빈도 단어 -> 주문 주기 (min_ms, max_ms)
bool FreqToRange(std::string_view freq, int& min_ms, int& max_ms) {
  if (freq == "high") {
    min_ms = 20, max_ms = 120;
  } else if (freq == "mid") {
    min_ms = 120, max_ms = 600;
  } else if (freq == "low") {
    min_ms = 600, max_ms = 2500;
  } else {
    return false;
  }
  return true;
}
}  // namespace

int main(int argc, char** argv) {
  std::vector<std::string_view> pos;
  for (int i = 1; i < argc; ++i) {
    std::string_view a = argv[i];
    if (a == "-d" || a == "--debug")
      logger::SetLogLevel(logger::LogLv::kDebug);
    else
      pos.push_back(a);
  }

  int min_ms = 0, max_ms = 0;
  if (pos.empty() || !FreqToRange(pos[0], min_ms, max_ms)) {
    std::cout << "usage: ./consumer <high|mid|low> [id] [-d|--debug]"
              << std::endl;
    return 1;
  }

  // 로그 태그용 id. 생략 시 빈도 대역명(pos[0])을 사용.
  std::string id{pos.size() >= 2 ? pos[1] : pos[0]};

  // 모든 consumer 가 log/consumer.log 한 파일에 기록 (라인별 consumer#<id> 태그)
  logger::InitFileLog("consumer");
  logger::SetInstanceId(id);

  asio::io_context io;
  Consumer consumer(io, kServerHost, kPortBase, min_ms, max_ms);

  asio::signal_set signals(io, SIGINT, SIGTERM);
  signals.async_wait([&](const boost::system::error_code&, int) {
    LOG_INFO("시그널 수신 - consumer 종료");
    consumer.Stop();
    io.stop();
  });

  LOG_INFO("consumer 시작 id={} 주기 {}~{}ms -> {}:{}", id, min_ms, max_ms,
           kServerHost, kPortBase + kOrderEntryPortOffset);

  io.run();
  return 0;
}
