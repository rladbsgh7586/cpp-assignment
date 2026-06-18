#!/usr/bin/env bash
# n명의 일반 소비자(consumer)를 랜덤 빈도로 띄우는 스크립트
#   usage: _run_consumers.sh <count>
# Ctrl-C 전체 consumer 를 일괄 kill
set -euo pipefail

N="${1:?usage: $0 <count>}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/build/output/consumer"
FREQS=(low mid high)

if [[ ! -x "$BIN" ]]; then
  echo "consumer 바이너리가 없습니다: $BIN (먼저 빌드하세요)" >&2
  exit 1
fi

# 로그(log/)는 cwd 기준으로 생성된다. client/server 와 동일하게 바이너리 위치
# (build/output)에서 실행해 로그를 build/output/log 한 곳으로 통일한다.
cd "$(dirname "$BIN")"

pids=()
cleanup() {
  echo
  echo "consumer 종료 중..."
  kill "${pids[@]}" 2>/dev/null || true
  wait 2>/dev/null || true
}
trap cleanup INT TERM EXIT

# 빈도 대역별 인덱스 (id = <빈도><번호>: low0, low1, mid0, high0 ...)
declare -A idx=([low]=0 [mid]=0 [high]=0)
for ((i = 1; i <= N; i++)); do
  freq="${FREQS[$((RANDOM % ${#FREQS[@]}))]}"
  id="${freq}${idx[$freq]}"
  idx[$freq]=$((idx[$freq] + 1))
  "$BIN" "$freq" "$id" >/dev/null 2>&1 &
  pids+=("$!")
done

echo "launched $N consumers (Ctrl-C 로 전체 종료) — 로그: $(dirname "$BIN")/log/consumer.log"
wait
