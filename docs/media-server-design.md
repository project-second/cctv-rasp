# RTSP 미디어 서버 설계

## 목표

`rail-media`는 Raspberry Pi에 연결된 PiCam 영상을 RTSP로 제공하는 C/C++ 프로그램이다.
1차 성공 기준은 VLC에서 다음 주소를 열었을 때 영상이 나오는 것이다.

```txt
rtsp://<pi-ip>:8554/live
```

## 범위

- PiCam 입력은 `libcamerasrc`를 사용한다.
- 미디어 파이프라인은 GStreamer와 `gst-rtsp-server`를 사용한다.
- 기본 출력은 RTSP다.
- PTZ, WebRTC, 녹화, 외부 웹 서버 통신은 이후 단계로 미룬다.

## 프로세스

```txt
rail-media
```

`rail-media` 하나가 다음 역할을 담당한다.

- PiCam 입력 받기
- H.264 인코딩
- RTP payloader 연결
- RTSP 서버 열기
- HTTP API로 영상 프로필 조회와 변경 처리

## 기본 파이프라인

Pi 4 계열에서 먼저 확인할 파이프라인은 다음 흐름이다.

```txt
libcamerasrc
→ video/x-raw,width=1280,height=720,framerate=30/1,format=NV12
→ v4l2h264enc
→ h264parse
→ rtph264pay
→ RTSP /live
```

Pi 5 또는 `v4l2h264enc`가 동작하지 않는 환경에서는 `x264enc`를 사용한다.

```txt
libcamerasrc
→ video/x-raw,width=1280,height=720,framerate=30/1,format=NV12
→ x264enc
→ h264parse
→ rtph264pay
→ RTSP /live
```

## 실행 옵션

`rail-media`는 다음 옵션을 받는다.

```txt
--port <port>       기본값: 8554
--control-port <p>  기본값: 8081
--mount <path>      기본값: /live
--profile <name>    low, main, high 중 하나, 기본값: main
--encoder <name>    v4l2 또는 x264, 기본값: v4l2
--width <pixels>    기본값: 1280
--height <pixels>   기본값: 720
--fps <fps>         기본값: 30
```

기본 프로필:

| Profile | Quality | Resolution | FPS | Encoder | Target Bitrate |
|---|---|---:|---:|---|---:|
| `low` | 360p | 640x360 | 30 | `v4l2` | 800 kbps |
| `main` | 720p | 1280x720 | 30 | `v4l2` | 2500 kbps |
| `high` | 1080p | 1920x1080 | 30 | `v4l2` | 5000 kbps |

Target Bitrate는 현재 운영 기준값이다.
현재 코드에는 bitrate 설정 필드가 없으므로 실제 인코더 bitrate 적용은 이후 `Config`와 `Profile`에 `bitrate_kbps`를 추가한 뒤 GStreamer encoder 옵션에 연결한다.

## 미디어 서버 후보 비교

현재 선택은 `GStreamer + gst-rtsp-server`다.

| 후보 | 장점 | 단점 | 현재 판단 |
|---|---|---|---|
| `GStreamer + gst-rtsp-server` | PiCam 입력을 `libcamerasrc`로 직접 받고, H.264 인코딩과 RTP payloader, RTSP `/live` 송출을 한 파이프라인에서 처리할 수 있다. `rail-media` 내부에서 프로필 변경 시 RTSP factory/pipeline을 직접 재생성할 수 있다. | C/C++와 GStreamer API를 직접 관리해야 한다. | 채택 |
| `FFmpeg` 단독 | 빠른 캡처와 송출 테스트가 쉽다. | 앱 내부에서 프로필 변경, RTSP 서버 상태 관리, PiCam/libcamera 제어를 세밀하게 통합하기 어렵다. | 현재 범위에서는 미채택 |
| `MediaMTX` | RTSP/HLS/WebRTC 중계 서버로 쓰기 좋고 외부 배포 기능이 많다. | PiCam 캡처와 프로필 변경은 별도 프로세스에서 처리해야 하므로 현재 `rail-media` 단일 프로세스 구조보다 복잡하다. | 추후 외부 중계 필요 시 검토 |
| WebRTC/HLS 중심 서버 | 브라우저 재생과 웹 배포에 유리하다. | 1차 목표인 VLC/RTSP 확인과 Qt/웹의 단순 프로필 변경 요구에는 과하다. | 추후 브라우저 직접 재생 요구 시 검토 |

따라서 현재 단계에서는 `rail-media` 단일 프로세스 안에서 `GStreamer + gst-rtsp-server`를 유지한다.

예시:

```bash
./rail-media --port 8554 --mount /live --encoder v4l2
./rail-media --port 8554 --mount /live --encoder x264
./rail-media --profile low
```

실행 중 프로필 변경은 HTTP API로 요청한다.

```bash
curl http://<pi-ip>:8081/profile
curl http://<pi-ip>:8081/profiles
curl -X POST http://<pi-ip>:8081/profile/high
```

프로필 변경 시 `rail-media` 프로세스는 유지되고 RTSP 연결만 잠깐 끊길 수 있다.
변경 후 RTSP 주소는 그대로 `rtsp://<pi-ip>:8554/live`를 사용한다.

## Raspberry Pi에서 먼저 확인할 것

```bash
rpicam-hello --list-cameras
gst-inspect-1.0 libcamerasrc
gst-inspect-1.0 v4l2h264enc
gst-inspect-1.0 x264enc
gst-inspect-1.0 rtph264pay
```

RTSP 서버 개발 패키지와 런타임도 필요하다.

```bash
gst-inspect-1.0 | grep rtsp
```

## 크로스 컴파일

PC/WSL에서 Raspberry Pi 4/5 64-bit용으로 빌드한다.

```bash
cmake -S media-server -B media-server/build/pi-release \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/raspi-aarch64.cmake" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build media-server/build/pi-release
```

자세한 순서는 [rail-media 크로스 컴파일 실행 문서](cross-compile-rail-media.md)를 따른다.

Raspberry Pi에는 다음 런타임/개발 패키지를 설치한다.

```bash
sudo apt install -y \
  build-essential \
  cmake \
  pkg-config \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev \
  libgstrtspserver-1.0-dev
```

실행:

```bash
/opt/rail-cctv/bin/rail-media
```

VLC에서 확인:

```txt
rtsp://<pi-ip>:8554/live
```

## 확인된 실행 명령과 설정값

현재 Raspberry Pi 주소는 다음으로 확인했다.

```txt
192.168.0.32
```

카메라와 GStreamer 플러그인 확인:

```bash
rpicam-hello --list-cameras
gst-inspect-1.0 libcamerasrc
gst-inspect-1.0 v4l2h264enc
gst-inspect-1.0 x264enc
gst-inspect-1.0 rtph264pay
dpkg -s libgstrtspserver-1.0-dev
```

GStreamer 테스트 파이프라인:

```bash
gst-launch-1.0 libcamerasrc \
  ! video/x-raw,width=1280,height=720,framerate=30/1,format=NV12 \
  ! v4l2h264enc extra-controls="controls,repeat_sequence_header=1" \
  ! video/x-h264,level=4 \
  ! h264parse \
  ! fakesink
```

`rail-media` 설치 위치와 실행 명령:

```bash
/opt/rail-cctv/bin/rail-media
```

확인된 기본 설정값:

```txt
RTSP 주소: rtsp://192.168.0.32:8554/live
HTTP 제어 API: http://192.168.0.32:8081
기본 프로필: main
기본 해상도/FPS: 1280x720, 30fps
기본 인코더: v4l2
RTSP mount path: /live
```

제어 API 확인:

```bash
curl http://192.168.0.32:8081/health
curl http://192.168.0.32:8081/profile
curl http://192.168.0.32:8081/profiles
curl -X POST http://192.168.0.32:8081/profile/low
curl -X POST http://192.168.0.32:8081/profile/main
curl -X POST http://192.168.0.32:8081/profile/high
```

## 로그 위치와 확인 방법

`rail-media`는 별도 로그 파일을 만들지 않는다.
표준 출력과 표준 에러로 로그를 남기고, systemd 서비스로 실행할 때 systemd journal에서 확인한다.

서비스명:

```txt
rail-media.service
```

실시간 로그 확인:

```bash
journalctl -u rail-media.service -f
```

최근 로그 확인:

```bash
journalctl -u rail-media.service -n 100
```

현재 부팅 이후 로그 확인:

```bash
journalctl -u rail-media.service -b
```

에러 중심 확인:

```bash
journalctl -u rail-media.service -p warning..alert
```

로그 포맷:

```txt
[info] rail-media started
[info] profile switched
[error] profile switch failed: unknown profile: ultra
[error] failed to bind HTTP control port: Address already in use
```

## 완료 기준

- `rail-media`가 에러 없이 실행된다.
- Raspberry Pi에서 `ss -lntp | grep 8554`로 RTSP 포트가 열린다.
- VLC에서 `rtsp://<pi-ip>:8554/live`로 영상이 나온다.
- 실행에 성공한 encoder와 해상도/FPS를 문서화한다.
