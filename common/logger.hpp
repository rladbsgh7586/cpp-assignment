// std::format 기반 로거 (C++20)
#pragma once

#include "pch.hpp"

namespace logger {

enum class LogLv {
  kNone = 0,
  kDebug = 1,
  kInfo = 2,
  kWarn = 3,
  kError = 4,
};

void SetLogLevel(LogLv level);
LogLv GetLogLevel();

// 호출시 log/<name>.log 파일에도 로그 출력
void InitFileLog(const std::string& name);

namespace detail {

// 포맷이 끝난 본문 + 위치를 실제로 콘솔/파일에 출력
void write(std::string_view header, std::string_view body,
           const std::source_location& loc);

// 레벨 체크 & 포맷팅
template <typename... Args>
void log(std::string_view header, LogLv level, std::source_location loc,
         std::format_string<Args...> fmt, Args&&... args) {
  if (level < GetLogLevel()) return;
  write(header, std::format(fmt, std::forward<Args>(args)...), loc);
}

}  // namespace detail

}  // namespace logger

// API -- 로깅 함수
#define LOG_DEBUG(...)                                \
  logger::detail::log("DEBUG", logger::LogLv::kDebug, \
                      std::source_location::current(), __VA_ARGS__)
#define LOG_INFO(...)                               \
  logger::detail::log("INFO", logger::LogLv::kInfo, \
                      std::source_location::current(), __VA_ARGS__)
#define LOG_WARN(...)                               \
  logger::detail::log("WARN", logger::LogLv::kWarn, \
                      std::source_location::current(), __VA_ARGS__)
#define LOG_ERROR(...)                                \
  logger::detail::log("ERROR", logger::LogLv::kError, \
                      std::source_location::current(), __VA_ARGS__)
