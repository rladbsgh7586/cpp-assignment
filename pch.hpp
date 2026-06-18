// precompiled header
#pragma once

/* ── POSIX / 시스템 콜 ─────────────────────────────── */
#include <sys/syscall.h>  // gettid (logger)
#include <termios.h>      // 터미널 raw 모드 (keyboard_input)
#include <unistd.h>

#include <csignal>

/* ── C 표준 라이브러리 ─────────────────────────────── */
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

/* ── C++ 표준 라이브러리 ───────────────────────────── */
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <random>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>

// 컨테이너
#include <array>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// 동시성
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

// boost 네트워크
#include <boost/asio.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

// 프로젝트 헤더 - 헤더 간 의존 순서를 수동 유지
// clang-format off
#include "common/definitions.hpp"
#include "common/keyboardinput.hpp"
#include "common/logger.hpp"
#include "common/protocol.hpp"
#include "common/matchengine.hpp"
#include "common/util.hpp"
#include "common/latencylog.hpp"
#include "common/baseclient.hpp"
// clang-format on
