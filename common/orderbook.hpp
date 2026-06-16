// 실제 주문이 발생하는 orderbook struct - 구현예정
// 각 호가에는 선입선출로 client의 주문이 등록되어있음.
// 호가의 범위는 protocol에 정의된 5개가 아닌 더 넓은 범위로 관리 (실제 매칭엔진이니)
// 단 전송하는 orderbook은 최우선 매수호가/매도호가에서 5layer씩만 추출하여 전송
#pragma once

#include "pch.hpp"