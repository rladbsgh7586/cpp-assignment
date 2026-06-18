// 공용 유틸리티 함수 모음.
#pragma once

#include "pch.hpp"

// Side -> 로그용 문자열.
std::string_view SideStr(Side s);

// 호가창을 사람이 읽기 좋은 표로 (ASK 위[멀리->가까이], BID 아래[가까이->멀리]).
std::string FormatBook(const Orderbook& b);

// 음이 아닌 정수 전체 파싱 (실패 시 false).
bool ParseNonNeg(std::string_view s, int& out);
