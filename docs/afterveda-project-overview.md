# Afterveda 프로젝트 상세 설명

## 개요

Afterveda는 Raspberry Pi 기반 CCTV 장비를 만들기 위한 C++ 프로젝트다. 현재 저장소는 카메라 영상을 RTSP로 송출하고, ONVIF 클라이언트가 장비를 검색하고 스트림 주소와 PTZ 제어를 사용할 수 있게 하는 데 초점을 둔다. 실제 영상 캡처와 송출은 `rail-media`, ONVIF 호환 인터페이스는 `afterveda-onvif`, Pan/Tilt 서보 제어는 `hardware-control`이 담당한다.

최종 런타임 형태는 다음과 같다.

```txt
Pi Camera
  -> rail-media
     -> GStreamer RTSP stream: rtsp://<pi-ip>:8554/live
     -> HTTP profile control: http://<pi-ip>:8081

ONVIF Client / VMS
  -> afterveda-onvif
     -> WS-Discovery UDP 3702
     -> ONVIF SOAP HTTP 8000
     -> GetStreamUri returns rail-media RTSP URI
     -> PTZ requests execute hardware-control

hardware-control
  -> PCA9685 I2C PWM driver
  -> Pan servo / Tilt servo
```

## 저장소 구조

```txt
afterveda/
├── CMakeLists.txt                 # 전체 프로젝트 통합 빌드 진입점
├── README.md                      # 프로젝트 목표와 기본 실행 예시
├── cmake/toolchains/              # Raspberry Pi 크로스 컴파일 toolchain
├── docs/                          # 설계, 체크리스트, 빌드/배포 관련 문서
├── hardware-control/              # PCA9685 기반 Pan/Tilt 서보 제어 CLI
├── media-server/                  # PiCam 캡처 및 RTSP 송출 서버
└── onvif-server/                  # ONVIF Discovery, Device, Media, PTZ 서비스
```

루트 `CMakeLists.txt`는 세 하위 프로젝트를 `add_subdirectory()`로 묶는다. 각 하위 디렉터리는 독립 실행 파일을 만들며, 설치 대상은 모두 `bin` 아래로 잡혀 있다.

## 핵심 구성 요소

### 1. media-server: RTSP 영상 송출

`media-server`는 `rail-media` 실행 파일을 만든다. 역할은 Pi Camera 입력을 받아 GStreamer RTSP 서버로 H.264 스트림을 제공하는 것이다.

주요 파일:

- `media-server/src/main.cpp`: GStreamer 초기화, 설정 파싱, RTSP 서버와 HTTP 제어 서버 시작
- `media-server/src/config.cpp`: CLI 옵션 파싱과 검증
- `media-server/src/pipeline.cpp`: `libcamerasrc` 기반 GStreamer launch pipeline 생성
- `media-server/src/media_server.cpp`: `gst-rtsp-server` 구성, mount factory 설치, 인증 설정, 프로필 교체
- `media-server/src/control_server.cpp`: 간단한 HTTP 제어 API 제공
- `media-server/src/profiles.cpp`: `low`, `main`, `high` 영상 프로필 정의

기본 RTSP 주소는 다음과 같다.

```txt
rtsp://<pi-ip>:8554/live
```

기본 프로필은 `main`이며 현재 정의된 프로필은 다음과 같다.

| 프로필 | 품질 | 해상도 | FPS | Bitrate | Encoder |
|---|---:|---:|---:|---:|---|
| `low` | 360p | 640x360 | 30 | 800 kbps | `v4l2` |
| `main` | 720p | 1280x720 | 30 | 2500 kbps | `v4l2` |
| `high` | 1080p | 1920x1080 | 30 | 5000 kbps | `v4l2` |

GStreamer pipeline은 `libcamerasrc`에서 NV12 raw frame을 받아 H.264로 인코딩하고 `rtph264pay`로 RTSP payload를 만든다. 인코더는 기본적으로 Raspberry Pi 하드웨어 인코더 계열인 `v4l2h264enc`를 사용하며, 옵션으로 `x264enc`도 선택할 수 있다.

HTTP 제어 API는 기본 `8081` 포트에서 동작한다.

```txt
GET  /health              # 상태 확인
GET  /profiles            # 사용 가능한 프로필 목록
GET  /profile             # 현재 프로필 조회
POST /profile/<name>      # low/main/high 중 하나로 프로필 변경
```

프로필 변경 시 RTSP media factory를 새 설정으로 다시 설치하고 기존 RTSP client를 닫아 새 연결에서 변경된 스트림이 적용되게 한다.

### 2. onvif-server: ONVIF 호환 계층

`onvif-server`는 `afterveda-onvif` 실행 파일을 만든다. 이 프로세스는 실제 영상을 만들지 않고, 이미 실행 중인 `rail-media`의 RTSP 스트림을 ONVIF 카메라처럼 노출한다.

주요 파일:

- `onvif-server/src/app.cpp`: 전체 실행 흐름, signal 처리, Discovery thread와 SOAP HTTP server 시작
- `onvif-server/src/config.cpp`: 장비 정보, 포트, RTSP URI, 인증, PTZ 옵션 파싱
- `onvif-server/src/discovery_server.cpp`: WS-Discovery UDP 3702 Probe/Resolve 응답
- `onvif-server/src/http_server.cpp`: gSOAP 기반 HTTP/SOAP server
- `onvif-server/src/soap_services.cpp`: ONVIF Device, Media, Media2, PTZ, Imaging, Events, OSD 응답 구현
- `onvif-server/src/media_model.cpp`: ONVIF에 노출할 media profile/model 구성
- `onvif-server/src/media_xml.cpp`: ONVIF Media XML 응답 생성
- `onvif-server/src/rail_client.cpp`: `rail-media` HTTP control API 호출
- `onvif-server/gsoap/`: ONVIF WSDL/XSD와 gSOAP 생성물

ONVIF discovery는 `239.255.255.250:3702` multicast 그룹에 참여하고, Probe 또는 Resolve 요청이 오면 `NetworkVideoTransmitter` 타입과 XAddr를 응답한다. XAddr는 보통 다음 형태다.

```txt
http://<xaddr-host>:8000/device_service
```

SOAP HTTP server는 기본 `8000` 포트에서 요청을 받는다. `/health` 요청에는 JSON 상태 응답을 제공하고, SOAP 요청은 gSOAP generated binding을 통해 각 `__tds__`, `__trt__`, `__tr2__`, `__tptz__` 계열 함수로 라우팅된다.

현재 구현 범위는 Profile T 지향 구현이다. Device Test Tool을 통과한 공식 Profile T conformant 상태로 단정하면 안 된다. 구현된 주요 기능은 다음과 같다.

- WS-Discovery `Probe`, `Resolve`
- Device: `GetCapabilities`, `GetDeviceInformation`, `GetServices`, `GetSystemDateAndTime`, `GetScopes`, `GetHostname`, `GetNetworkInterfaces`, `GetUsers`, `GetServiceCapabilities`
- Media1: `GetProfiles`, `GetStreamUri`, `GetVideoEncoderConfiguration`, `SetVideoEncoderConfiguration`, video source/configuration 관련 조회
- Media2: 기본 H.264 profile, stream URI, service capabilities
- PTZ: node/configuration/preset/status 조회, preset/home 이동, continuous/relative move, stop
- Imaging/Events/OSD: 클라이언트 호환성을 위한 기본 응답
- 선택적 ONVIF UsernameToken 인증: `PasswordText`, `PasswordDigest`

`GetStreamUri`는 자체 스트림을 만들지 않고 `--rtsp-uri`로 받은 값을 응답한다. UDP multicast 요청은 현재 지원하지 않고 RTP/RTSP/TCP unicast 중심으로 동작한다.

PTZ 요청은 `hardware-control` 바이너리를 shell command로 실행한다. `ContinuousMove` 또는 `RelativeMove`의 X/Y 값 방향을 읽어 `left`, `right`, `up`, `down`, `stop` 중 하나로 변환한다. `GotoPreset`, `SetHomePosition`, `GotoHomePosition`은 `center` 명령으로 처리한다.

### 3. hardware-control: Pan/Tilt 서보 제어

`hardware-control`은 Raspberry Pi에서 PCA9685 I2C PWM 드라이버를 통해 Pan/Tilt 서보를 제어하는 CLI 도구다.

주요 파일:

- `hardware-control/src/main.cpp`: 설정 파싱, 현재 위치 로드, 명령 적용, 서보 출력, 상태 저장
- `hardware-control/src/config.cpp`: CLI 옵션과 범위 검증
- `hardware-control/src/pan_tilt.cpp`: `left/right/up/down/center/stop` 명령을 각도 변화로 변환
- `hardware-control/src/servo_controller.cpp`: 각도를 PCA9685 PWM count로 변환
- `hardware-control/src/pca9685.cpp`: Linux I2C device 접근, PCA9685 초기화, PWM register write
- `hardware-control/src/state_store.cpp`: 마지막 Pan/Tilt 각도를 파일에 저장

기본 하드웨어 매핑은 다음과 같다.

| 항목 | 기본값 |
|---|---|
| I2C device | `/dev/i2c-1` |
| PCA9685 address | `0x40` |
| Pan channel | `0` |
| Tilt channel | `1` |
| Pan range | `30..150` degrees |
| Tilt range | `45..135` degrees |
| Center | `90` degrees |
| Step | `5` degrees |
| State file | `/tmp/rail-hardware-control.state` |

명령은 매번 한 번 실행되고 종료된다. 마지막 각도는 state file에 저장되므로 다음 명령은 이전 위치를 기준으로 움직인다. `stop`은 모터 전원 차단이 아니라 현재 위치를 다시 적용하는 동작이다.

`--dry-run`을 쓰면 I2C 장치에 접근하지 않고 계산된 각도와 PWM count만 출력한다. ONVIF PTZ 연동 테스트에서도 `afterveda-onvif --ptz-dry-run`을 사용하면 실제 하드웨어 없이 흐름을 확인할 수 있다.

## 프로세스 실행 순서

일반적인 실행 순서는 다음과 같다.

```bash
media-server/build/rail-media --profile main
```

```bash
onvif-server/build/afterveda-onvif \
  --device-name afterveda-camera \
  --xaddr-host <pi-ip> \
  --rtsp-uri rtsp://<pi-ip>:8554/live \
  --rail-control-url http://127.0.0.1:8081 \
  --hardware-control hardware-control/build/hardware-control
```

하드웨어 없이 PTZ만 검증하려면 다음 옵션을 추가한다.

```bash
--ptz-dry-run
```

ONVIF 인증이 필요하면 `afterveda-onvif`에 다음 옵션을 함께 준다.

```bash
--username <user> --password <password>
```

RTSP 자체에도 Digest 인증을 걸 수 있다.

```bash
media-server/build/rail-media \
  --rtsp-user <user> \
  --rtsp-password <password>
```

단, ONVIF `GetStreamUri`가 반환하는 URI와 RTSP 인증 정책을 클라이언트가 함께 처리할 수 있는지 별도로 확인해야 한다.

## 빌드 방식

개발 PC에서 전체 빌드:

```bash
cmake -S . -B build
cmake --build build
```

하위 프로젝트만 빌드:

```bash
cmake -S media-server -B media-server/build
cmake --build media-server/build

cmake -S onvif-server -B onvif-server/build
cmake --build onvif-server/build

cmake -S hardware-control -B hardware-control/build
cmake --build hardware-control/build
```

Raspberry Pi 4/5 64-bit 대상 크로스 컴파일은 `cmake/toolchains/raspi-aarch64.cmake`를 사용한다.

```bash
cmake -S media-server -B media-server/build/pi-release \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/raspi-aarch64.cmake" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build media-server/build/pi-release
```

## 외부 의존성

### media-server

- C++17
- GStreamer 1.0
- `gst-rtsp-server-1.0`
- GIO / GLib
- Raspberry Pi camera stack에서 사용할 수 있는 `libcamerasrc`
- H.264 encoder: `v4l2h264enc` 또는 `x264enc`

### onvif-server

- C/C++17
- gSOAP runtime: `stdsoap2.h`, `libgsoap++` 또는 `libgsoap`
- gSOAP plugins: `wsddapi`, `wsaapi`
- pthread / CMake Threads
- 생성된 ONVIF binding 파일: `onvif-server/gsoap/generated/*`

### hardware-control

- C++17
- Linux I2C device: `/dev/i2c-1`
- PCA9685 PWM driver
- Pan/Tilt 서보와 외부 5V 서보 전원

## 주요 데이터 흐름

### 영상 스트림 흐름

```txt
PiCam
  -> libcamerasrc
  -> raw NV12 frames
  -> H.264 encoder
  -> rtph264pay
  -> gst-rtsp-server
  -> rtsp://<pi-ip>:8554/live
```

### ONVIF 검색 흐름

```txt
ONVIF Client
  -> WS-Discovery Probe multicast 239.255.255.250:3702
  -> afterveda-onvif ProbeMatch
  -> Client calls http://<xaddr-host>:8000/device_service
  -> GetServices / GetProfiles / GetStreamUri
  -> Client opens RTSP URI from rail-media
```

### PTZ 제어 흐름

```txt
ONVIF Client PTZ command
  -> afterveda-onvif SOAP PTZ handler
  -> hardware-control CLI command
  -> load previous pan/tilt state
  -> clamp angle to configured range
  -> convert angle to PCA9685 PWM count
  -> write I2C PWM registers
  -> save new state
```

### 프로필 변경 흐름

```txt
ONVIF SetVideoEncoderConfiguration or HTTP POST /profile/<name>
  -> rail-media control API
  -> apply low/main/high profile
  -> rebuild RTSP media factory
  -> close existing RTSP clients
  -> new clients receive updated stream
```

## 현재 구현상 주의점

- ONVIF는 Profile T 지향 구현이지만 공식 인증 완료 상태는 아니다.
- Imaging 설정은 호환 응답 중심이며 실제 센서 제어로 연결되어 있지 않다.
- OSD 응답은 기본 호환 응답이며 실제 영상 overlay pipeline은 아직 구현되지 않았다.
- Metadata streaming은 아직 완성되지 않았다.
- PTZ는 continuous velocity를 실제 속도 제어로 처리하지 않고 방향별 step 명령으로 변환한다.
- PTZ 명령은 `std::system()`으로 `hardware-control`을 실행하므로 경로와 인자 escaping, 실행 권한, 배포 위치를 신경 써야 한다.
- `hardware-control stop`은 즉시 정지나 전원 차단이 아니라 현재 각도를 유지하도록 PWM을 다시 쓰는 동작이다.
- `rail-media`의 HTTP control server는 단순 HTTP parser이므로 reverse proxy 수준의 복잡한 HTTP 기능을 기대하면 안 된다.
- RTSP UDP multicast는 현재 ONVIF 응답에서 지원하지 않는 것으로 처리된다.

## 개발/검증 체크리스트

1. Raspberry Pi에서 카메라가 인식되는지 확인한다.
2. `rail-media`를 실행하고 VLC에서 `rtsp://<pi-ip>:8554/live`를 확인한다.
3. `curl http://<pi-ip>:8081/health`, `/profiles`, `/profile`로 control API를 확인한다.
4. `POST /profile/low`, `main`, `high`로 프로필 전환을 확인한다.
5. `hardware-control --dry-run center/left/right/up/down`으로 각도와 PWM count를 확인한다.
6. 실제 PCA9685 연결 후 `i2cdetect -y 1`로 `0x40` 주소를 확인한다.
7. 실제 서보 연결 전 `--pan-min`, `--pan-max`, `--tilt-min`, `--tilt-max`를 안전 범위로 잡는다.
8. `afterveda-onvif --ptz-dry-run`으로 ONVIF discovery, media, PTZ 흐름을 먼저 확인한다.
9. ONVIF 클라이언트 또는 VMS에서 장비 검색, `GetStreamUri`, RTSP 재생, PTZ 버튼 동작을 확인한다.
10. systemd 배포 시 `rail-media`가 먼저 뜨고 `afterveda-onvif`가 그 RTSP URI를 노출하도록 의존성을 잡는다.

## 한 줄 요약

Afterveda는 `rail-media`가 카메라 영상을 RTSP로 송출하고, `afterveda-onvif`가 그 RTSP 스트림을 ONVIF 카메라로 노출하며, `hardware-control`이 ONVIF PTZ 요청을 Raspberry Pi I2C/PCA9685 기반 Pan/Tilt 서보 제어로 연결하는 구조다.
