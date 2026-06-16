#include "pch.hpp"

namespace {
logger::LogLv s_globalLogLevel = logger::LogLv::kDebug;
std::mutex s_logMutex;
std::ofstream s_logFile;

long current_tid() { return static_cast<long>(syscall(SYS_gettid)); }

// "YYYYMMDD-HHMMSS" 로컬타임 timestamp
std::string get_date_time() {
  auto now = std::chrono::floor<std::chrono::seconds>(
      std::chrono::system_clock::now());
  return std::format("{:%Y%m%d-%H%M%S}",
                     std::chrono::current_zone()->to_local(now));
}

// source_location 의 전체 경로에서 파일명만 추출
std::string_view base_name(std::string_view path) {
  auto pos = path.rfind('/');
  return pos == std::string_view::npos ? path : path.substr(pos + 1);
}

// function_name()에서 "Class::func" 부분만 추출
std::string_view extract_func(std::string_view pretty) {
  auto rparen = pretty.rfind('(');
  if (rparen == std::string_view::npos) return pretty;
  auto space = pretty.rfind(' ', rparen);
  auto start = (space == std::string_view::npos) ? 0 : space + 1;
  return pretty.substr(start, rparen - start);
}

}  // namespace

namespace logger {

void SetLogLevel(LogLv level) { s_globalLogLevel = level; }
LogLv GetLogLevel() { return s_globalLogLevel; }

void InitFileLog(const std::string& name) {
  std::lock_guard<std::mutex> lk(s_logMutex);
  std::filesystem::create_directories("log");
  std::string path = "log/" + name + ".log";
  s_logFile.open(path, std::ios::out | std::ios::app);
}

namespace detail {

void write(std::string_view header, std::string_view body,
           const std::source_location& loc) {
  std::string line =
      std::format("{} [ {:>6} ]  [{:>6}] {} on {}:{}-{}", get_date_time(),
                  header, current_tid(), body, base_name(loc.file_name()),
                  loc.line(), extract_func(loc.function_name()));

  std::lock_guard<std::mutex> lk(s_logMutex);
  std::printf("%s\n", line.c_str());

  if (s_logFile.is_open()) {
    s_logFile << line << '\n';
    s_logFile.flush();
  }
}

}  // namespace detail

}  // namespace logger
