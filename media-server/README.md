# 미디어 서버

카메라 입력과 RTSP 영상 송출을 정리하는 영역이다.

## 담당 범위

- 카메라 장치 인식
- FFmpeg 또는 GStreamer 기반 캡처 테스트
- RTSP 송출 구성
- VLC 또는 외부 클라이언트 확인
- 해상도, FPS, bitrate 설정 정리
- RTSP Digest 인증과 `libsoup` 기반 HTTP 제어 API 정리

## 첫 작업

- 사용할 카메라 종류 확인 완료: PiCam
- 장치 인식과 GStreamer 입력 확인 완료
- RTSP 송출 방식 결정: GStreamer + gst-rtsp-server
- 성공한 스트리밍 명령어 문서화

## 관련 문서

- [RTSP 미디어 서버 설계](../docs/media-server-design.md)
- [프로필 관리자 계획](../docs/profile-manager-plan.md)
- [PiCam 크로스 컴파일 작업 순서](../docs/picam-cross-compile-flow.md)

## 크로스 컴파일

PC/WSL에서 Raspberry Pi 4/5 64-bit용으로 빌드한다.

```bash
cmake -S media-server -B media-server/build/pi-release \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/raspi-aarch64.cmake" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build media-server/build/pi-release
```

자세한 순서는 [rail-media 크로스 컴파일 실행 문서](../docs/cross-compile-rail-media.md)를 따른다.

크로스 컴파일 sysroot에는 GStreamer/gst-rtsp-server와 함께 `libsoup-3.0-dev`, `libjson-glib-dev`의 헤더, 라이브러리, `pkgconfig` 파일이 필요하다.

## Raspberry Pi 실행

```bash
/opt/rail-cctv/bin/rail-media
```

ONVIF에는 안정적인 `main` 미디어 프로필 하나를 제공하며 기본 영상 설정은 720p/30fps다. 실행 시 `low`, `main`, `high`는 프로필 식별자가 아니라 시작 화질 프리셋으로 사용한다.

```bash
/opt/rail-cctv/bin/rail-media --profile low
/opt/rail-cctv/bin/rail-media --profile main
/opt/rail-cctv/bin/rail-media --profile high
```

주요 옵션:

```txt
--profile <name>      시작 화질 프리셋 low, main, high 중 하나. 기본값: main
--port <port>         RTSP 포트. 기본값: 8554
--control-host <ip>   HTTP 제어 API 바인드 주소. 기본값: 127.0.0.1
--control-port <p>    HTTP 제어 포트. 기본값: 8081
--mount <path>        RTSP mount path. 기본값: /live
--encoder <name>      v4l2 또는 x264. 기본값: v4l2
--bitrate-kbps <k>    목표 bitrate. 기본값: 프로필 값
--osd-text <text>     영상 위에 표시할 OSD 텍스트. 기본값: Afterveda
--no-osd              OSD 텍스트 표시 비활성화
--rtsp-user <user>    RTSP Digest 인증 사용자명
--rtsp-password <p>   RTSP Digest 인증 비밀번호
--rtsp-realm <name>   RTSP Digest 인증 realm. 기본값: rail-media
```

VLC 접속 주소:

```txt
rtsp://<pi-ip>:8554/live
```

현재 ONVIF 프로필 설정과 화질 프리셋 조회·적용:

```bash
curl http://127.0.0.1:8081/health
curl http://127.0.0.1:8081/profile
curl http://127.0.0.1:8081/profiles
curl http://127.0.0.1:8081/presets
curl -X POST http://127.0.0.1:8081/preset/low
curl -X POST http://127.0.0.1:8081/preset/main
curl -X POST http://127.0.0.1:8081/preset/high
```

`GET /profiles`는 실제 ONVIF 프로필 하나와 현재 encoder 설정을 반환한다. `/presets`와 `/preset/<name>`은 편의를 위한 화질 프리셋 API다. 이전 `/profile/<name>` 주소도 호환 목적으로 계속 지원한다. 설정을 바꿔도 프로필 토큰 `main`과 encoder configuration 토큰 `encoder-main`은 유지된다.

`/health`는 RTSP media pipeline이 클라이언트 연결 시 지연 생성되는 구조를 반영한다.

- `idle` / `not_checked`: RTSP 서버는 준비됐지만 아직 스트림 연결이 없어 카메라 pipeline을 실행하지 않은 상태
- `preparing` / `checking`: 클라이언트 연결 후 카메라 pipeline을 준비하는 상태
- `ok` / `ready` / `camera: ok`: pipeline 준비가 끝나 스트리밍할 수 있는 상태
- `error`: RTSP 서버 또는 pipeline 준비 실패 상태이며 HTTP `503` 반환

응답에는 현재 profile, resolution, FPS, bitrate와 함께 다음 운영 정보도 포함된다.

- `started_at`, `uptime_seconds`: 프로세스 시작 시각과 실행 시간
- `last_pipeline_ready_at`: 가장 최근 pipeline 준비 완료 시각
- `rtsp_clients`: 현재 연결된 RTSP client 수
- `last_error`: 가장 최근 GStreamer 오류 또는 `null`

이미지 설정 조회와 변경:

```bash
curl http://127.0.0.1:8081/imaging
curl -X POST http://127.0.0.1:8081/imaging -d '{"brightness":70}'
curl -X POST http://127.0.0.1:8081/imaging -d '{"contrast":55,"color_saturation":60}'
```

OSD 조회와 변경:

```bash
curl http://127.0.0.1:8081/osd
curl -X POST http://127.0.0.1:8081/osd -d '{"enabled":true,"text":"Afterveda"}'
curl -X POST http://127.0.0.1:8081/osd -d '{"enabled":false}'
```

외부 장비에서 제어 API를 직접 호출해야 할 때만 `--control-host 0.0.0.0`으로 실행한다.
