# Profile Manager

이 문서는 `rail-media` Profile Manager 동작을 정리한다.

## 목표

Profile Manager는 Qt앱이나 웹 인터페이스에서 요청한 영상 프로필로 `rail-media`의 RTSP 스트림 설정을 변경하는 기능이다.

v1 목표는 다음 흐름을 지원하는 것이다.

```txt
Qt앱 또는 웹 인터페이스
→ HTTP API 요청
→ rail-media Profile Manager
→ RTSP 파이프라인 재생성
→ 같은 RTSP 주소로 재접속
```

RTSP 주소는 프로필이 바뀌어도 유지한다.

```txt
rtsp://<pi-ip>:8554/live
```

## v1 범위

- `rail-media` 프로세스 안에 Profile Manager를 둔다.
- HTTP API로 프로필 조회와 변경 요청을 받는다.
- 프로필 변경 시 기존 RTSP 스트림은 잠깐 끊겨도 된다.
- 변경 후 클라이언트는 같은 RTSP 주소로 다시 접속한다.
- 기본 인코더는 현재 성공한 Pi 4 하드웨어/드라이버 기반 `v4l2`로 둔다.

## 기본 프로필

| 이름 | 해상도 | FPS | 인코더 | 용도 |
|---|---:|---:|---|---|
| `low` | 640x360 | 30 | `v4l2` | 낮은 대역폭, 빠른 확인 |
| `main` | 1280x720 | 30 | `v4l2` | 기본 스트리밍 |
| `high` | 1920x1080 | 30 | `v4l2` | 고해상도 확인 |

초기 기본 프로필은 `main`으로 둔다.

`low`, `main`, `high`는 각각 360p, 720p, 1080p 해상도 선택이다.
FPS는 모두 30fps로 유지한다.

## HTTP API

기본 제어 포트는 `8081`로 둔다.

```txt
GET  /health
GET  /profiles
GET  /profile
POST /profile/low
POST /profile/main
POST /profile/high
```

예시:

```bash
curl http://<pi-ip>:8081/health
curl http://<pi-ip>:8081/profiles
curl http://<pi-ip>:8081/profile
curl -X POST http://<pi-ip>:8081/profile/low
curl -X POST http://<pi-ip>:8081/profile/main
curl -X POST http://<pi-ip>:8081/profile/high
```

`GET /profile` 응답 예시:

```json
{
  "profile": "main",
  "width": 1280,
  "height": 720,
  "fps": 30,
  "encoder": "v4l2"
}
```

알 수 없는 프로필을 요청하면 `400` 또는 `404`로 실패시킨다.

## RTSP 파이프라인 전환 방식

v1에서는 무중단 전환을 목표로 하지 않는다.

프로필 변경 요청이 오면 다음 순서로 처리한다.

```txt
1. 요청한 profile 이름을 검증한다.
2. 현재 RTSP factory 또는 pipeline을 중지한다.
3. 새 profile 값으로 GStreamer launch pipeline을 다시 만든다.
4. 같은 mount path `/live`에 새 pipeline을 등록한다.
5. 현재 profile 상태를 갱신한다.
6. 클라이언트는 같은 RTSP 주소로 다시 접속한다.
```

프로필 변경 중에는 영상이 잠깐 끊길 수 있다.

## 테스트 방법

기본 실행:

```bash
/opt/rail-cctv/bin/rail-media
```

시작 프로필 선택:

```bash
/opt/rail-cctv/bin/rail-media --profile low
/opt/rail-cctv/bin/rail-media --profile main
/opt/rail-cctv/bin/rail-media --profile high
```

아무 옵션 없이 실행하면 `main`으로 시작한다.

프로필 조회:

```bash
curl http://<pi-ip>:8081/profiles
curl http://<pi-ip>:8081/profile
```

프로필 변경:

```bash
curl -X POST http://<pi-ip>:8081/profile/low
curl -X POST http://<pi-ip>:8081/profile/main
curl -X POST http://<pi-ip>:8081/profile/high
```

RTSP 확인:

```txt
rtsp://<pi-ip>:8554/live
```

확인할 것:

- 프로필 변경 요청 후 `rail-media`가 죽지 않는다.
- 변경 후 같은 RTSP 주소로 다시 접속할 수 있다.
- `low`, `main`, `high`에서 해상도/FPS가 의도대로 바뀐다.
- 잘못된 프로필 이름은 실패한다.

## 나중으로 미룰 항목

- 무중단 프로필 전환
- 설정 파일 저장
- WebSocket 제어
- 인증/권한 처리
- 여러 RTSP mount 동시 제공
- 녹화 프로필과 스트리밍 프로필 분리
- 외부 웹 서버와 상태 동기화

## 남은 확인 항목

- VLC 같은 실제 RTSP 클라이언트가 프로필 변경 후 같은 주소로 재접속하는지 확인한다.
- PiCam이 `high` 프로필에서 안정적으로 동작하는지 사전 테스트한다.
- systemd 적용 후 API 포트 `8081`도 방화벽/네트워크에서 접근 가능한지 확인한다.
