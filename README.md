# C++ Programming Assignment

호가창을 발행하는 TCP 서버와 orderbook을 보고 주문을 내는 클라이언트로 구성.  
consumer와 매칭엔진을 구현하여 실제 주문을 처리, 시장을 모방하였다.  
C++20 STL + Boost.Asio(코루틴)만 사용.

세부적인 설계 의도는 각 소스 주석 참고

<br>
<br>

## 구조

크게 server, client, consumer, common 으로 나뉜다.

- **server** — 호가창 발행, 주문 송수신, 매칭엔진루프
- **client** — Q·N 전략 클라이언트
- **consumer** — 랜덤 빈도, 포지션, 양으로 주문 던지는 일반 참가자. 호가창에 유동성 채우는 용도
- **common** — 매칭엔진, 프로토콜, 로거 등 공통 모듈들
- **scripts** — `run_consumers.sh` (N consumer run/kill),
  `latency_stats.py` (latency 통계 출력용)
- **results** — latency 측정 결과 (개선 전/후)

<br>
<br>

## 빌드

CMake ≥ 3.20, g++ (C++20), Boost(asio, system) 필요.

```bash
cmake -B build && cmake --build build
```

실행파일 - `build/output/` 에 생성

<br>
<br>

## 실행

접속 host/port 는 `common/protocol.hpp` 참고 — `kServerHost`(127.0.0.1),
`kPortBase`(9000). 시장데이터가 9000, 주문이 9001.

```bash
# 서버
./build/output/server

# 전략 클라이언트 (Q=10, N=1)
./build/output/client 10 1

# 일반 참가자 N명
./scripts/run_consumers.sh 10
```

- `server` / `client` 는 console에 `q` 입력하여 종료, consumer 는 스크립트에서 Ctrl-C.
- 로그는 콘솔 또는 `log/<프로그램>.log` 에서 확인가능
- `server` / `client` 에 `-d` 플래그 붙힐시 호가창 변화 로그로 출력

<br>
<br>

## 측정 환경

| | |
| --- | --- |
| CPU | AMD Ryzen 5 5600X 중 4 vCPU (1 thread/core), KVM |
| Memory | 8 GiB |
| OS | Ubuntu 24.04.4 LTS (kernel 6.8) |

<br>
<br>

## 서버 처리 개선 시도

서버처리 개선 정도를 파악하기 위해 주문 발행(client) → 매칭엔진(server) → 주문 수신(client) 까지의 latency를 측정.

`client` 를 `-d` 플래그 포함하여 실행시 주문처리 latency 가 `log/latency_<id>.txt` 에 쌓임

`scripts/latency_stats.py` 로 latency 통계 확인 가능

### 매칭엔진 스레드

단일 io 스레드에서 같이 돌던 매칭엔진 + 통신에서 매칭엔진만 `jthread`로 분리,
통신모듈 → 매칭엔진으로 `ConcurrentQueue`로 전달.

3회 평균, 단위 µs (`result_n1000.txt` 취합):

**consumer 10**

| | before | after |
| --- | ---: | ---: |
| median | 232 | 295 |
| p90 | 528 | 618 |
| p99 | 40,361 | 28,835 ※ |

**consumer 100**

| | before | after |
| --- | ---: | ---: |
| median | 1,933 | 2,628 |
| p90 | 10,972 | 16,735 |
| p99 | 42,650 | 43,401 |

기대와 다르게 전반적으로는 오히려 느려짐

- consumer 10 의 p99 만 좋아졌는데, 이것도 편차가커 추가적인 측정이 필요하다 (ex) p95, consumer 명수 변경)

<br>

### 그 밖 개선

아래 시도는 전반적으로 느려짐 (구현 이슈일 수 있음)

- **lock-free SPSC + false sharing 방지** — 통신모듈 → 매칭엔진 전달을 lock-free
  링버퍼로 변경, head/tail 접근에서 발생하는 false sharing 은 `alignas(64)` 활용
- **큐를 busy polling 으로 소비** — cv 로 자고 깨는 context switch 비용 제거 목적
