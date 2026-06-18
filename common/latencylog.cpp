#include "pch.hpp"

LatencyLog::LatencyLog(int id) {
  std::filesystem::create_directories("log");
  std::string path = std::format("log/latency_{}.txt", id);
  file_.open(path, std::ios::out | std::ios::app);
  if (!file_.is_open()) {
    LOG_WARN("latency 로그 파일 열기 실패: {}", path);
  }
}

void LatencyLog::Record(int64_t send_epoch_ns, Side side, int price,
                        int64_t latency_ns) {
  if (!file_.is_open()) return;
  // 한 줄 = 한 주문. 공백 구분 raw record.
  file_ << send_epoch_ns << ' ' << SideStr(side) << ' ' << price << ' '
        << latency_ns << '\n';
}
