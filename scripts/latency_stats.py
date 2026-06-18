#!/usr/bin/env python3
"""주문 round-trip latency 통계.

client 가 debug(-d) 모드로 기록한 log/latency_<id>.txt 를 읽어 통계를 낸다.
파일 포맷 (한 줄 = 한 주문):
    <send_epoch_ns> <BID|ASK> <price> <latency_ns>

사용법:
    python3 scripts/latency_stats.py log/latency_3.txt
    python3 scripts/latency_stats.py log/latency_3.txt --n 1000
    python3 scripts/latency_stats.py log/latency_3.txt --unit us

표준 라이브러리만 사용 (외부 의존성 없음).
"""

import argparse
import statistics
import sys


def parse_latencies(path, n):
    """파일에서 latency_ns(마지막 필드)를 최대 n개까지 읽어 리스트로 반환."""
    values = []
    with open(path, "r") as f:
        for lineno, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue
            parts = line.split()
            try:
                values.append(int(parts[-1]))
            except (IndexError, ValueError):
                print(f"경고: {lineno}번째 줄 파싱 실패, 건너뜀: {line!r}",
                      file=sys.stderr)
                continue
            if n is not None and len(values) >= n:
                break
    return values


def percentile(sorted_vals, pct):
    """nearest-rank 백분위수 (sorted_vals 는 오름차순 정렬되어 있어야 함)."""
    if not sorted_vals:
        return 0
    k = max(0, min(len(sorted_vals) - 1,
                   int(round(pct / 100.0 * (len(sorted_vals) - 1)))))
    return sorted_vals[k]


# 단위 변환: ns 기준 나눗셈 값
UNITS = {"ns": 1, "us": 1_000, "ms": 1_000_000}


def main():
    ap = argparse.ArgumentParser(description="주문 round-trip latency 통계")
    ap.add_argument("file", help="latency txt 파일 경로")
    ap.add_argument("--n", type=int, default=None,
                    help="처음 N개 레코드만 사용 (기본: 전체)")
    ap.add_argument("--unit", choices=UNITS.keys(), default="us",
                    help="출력 단위 (기본: us)")
    args = ap.parse_args()

    try:
        latencies = parse_latencies(args.file, args.n)
    except OSError as e:
        print(f"파일 열기 실패: {e}", file=sys.stderr)
        return 1

    if not latencies:
        print("유효한 레코드가 없습니다.", file=sys.stderr)
        return 1

    div = UNITS[args.unit]
    sorted_ns = sorted(latencies)

    def fmt(ns):
        return f"{ns / div:,.3f}"

    count = len(latencies)
    mean = statistics.fmean(latencies)
    stdev = statistics.pstdev(latencies) if count > 1 else 0.0

    print(f"파일      : {args.file}")
    print(f"표본 수   : {count}  (단위: {args.unit})")
    print(f"min       : {fmt(sorted_ns[0])}")
    print(f"mean      : {fmt(mean)}")
    print(f"median    : {fmt(statistics.median(latencies))}")
    print(f"p50       : {fmt(percentile(sorted_ns, 50))}")
    print(f"p90       : {fmt(percentile(sorted_ns, 90))}")
    print(f"p99       : {fmt(percentile(sorted_ns, 99))}")
    print(f"max       : {fmt(sorted_ns[-1])}")
    print(f"stddev    : {fmt(stdev)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
