# Afterveda 미디어 애플리케이션

이 프로젝트의 범위는 CCTV RTSP 송출과 ONVIF 애플리케이션 구성이다. Raspberry Pi GPIO/PWM 같은 보드 의존 제어는 형제 폴더 `afterveda-bsp/`에서 관리한다.

## 목표

- Raspberry Pi 또는 CCTV 장비에서 카메라 영상 입력 확인
- RTSP 기반 영상 송출 구성
- ONVIF 기반 장비 검색과 RTSP 스트림 연동
- 필요 시 HLS 변환 또는 외부 미디어 서버 연동 구조 정리
- 외부 BSP 커널 모듈과 ONVIF PTZ 연동
- systemd 기반 자동 실행 및 로그 확인 방식 정리

## 프로젝트 구조

```txt
workspace/
├── afterveda/         # RTSP/ONVIF 애플리케이션
│   ├── media-server/  # 카메라 입력, RTSP 송출, 미디어 서버 구성
│   ├── onvif-server/  # ONVIF 검색, 장치/미디어/PTZ 서비스
│   ├── cmake/         # Raspberry Pi 크로스 컴파일 toolchain
│   ├── docs/          # 계획과 아키텍처 문서
│   └── README.md
└── afterveda-bsp/
    └── ptz-kmod/      # Raspberry Pi GPIO18/19 PWM PTZ 커널 모듈
```

## 우선순위

```txt
카메라 인식
→ RTSP 송출
→ ONVIF 검색/스트림 URI 제공
→ VLC에서 스트림 확인
→ 하드웨어 제어 신호 테스트
→ PTZ 동작 확인
→ systemd 자동 실행
→ 로그/재시작/장애 대응 정리
```

## 1차 목표

```txt
CCTV 카메라
→ Raspberry Pi / 엣지 장비
→ 미디어 서버
→ RTSP 스트림
→ ONVIF 장치/미디어 검색
→ VLC 또는 외부 클라이언트에서 확인
```

## Raspberry Pi 전체 빌드

`rail-media`와 `afterveda-onvif`를 한 번에 Raspberry Pi 4/5 64-bit용으로 빌드한다. 이 빌드는 `sysroot/raspi-aarch64`의 GStreamer/libsoup/json-glib/gSOAP 헤더와 라이브러리를 사용한다.

```bash
cmake --preset pi-release
cmake --build --preset pi-release
```

같은 설정을 프리셋 없이 실행하려면 다음 명령을 사용한다.

```bash
cmake -S . -B build/pi-release \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/raspi-aarch64.cmake" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build/pi-release
```

빌드 결과:

```txt
build/pi-release/media-server/rail-media
build/pi-release/onvif-server/afterveda-onvif
```

## ONVIF 실행 예시

`rail-media`를 먼저 실행한 뒤 별도 프로세스로 ONVIF 서버를 실행한다.

```bash
build/pi-release/media-server/rail-media --profile main

build/pi-release/onvif-server/afterveda-onvif \
  --xaddr-host <pi-ip> \
  --rtsp-uri rtsp://<pi-ip>:8554/live \
  --rail-control-url http://127.0.0.1:8081 \
  --ptz-device /dev/afterveda_ptz \
  --ptz-speed 30
```

ONVIF 서버는 검색/제어를 담당하고, 실제 영상은 기존 `rail-media` RTSP 스트림을 그대로 사용한다. PTZ 요청은 `--ptz-device`로 지정한 커널 모듈 문자 장치에 직접 기록한다. `ContinuousMove`는 `pan_speed=<deg/s> tilt_speed=<deg/s>`, `Stop`은 `stop`, `RelativeMove`/home/preset 이동은 `pan=<deg> tilt=<deg>` 명령으로 연결된다.

## 현재 구현 요약

- `rail-media`: PiCam `libcamerasrc` 입력, H.264 RTSP `/live` 송출, 로컬 HTTP 제어 API `127.0.0.1:8081`
- `afterveda-onvif`: WS-Discovery UDP `3702`, ONVIF SOAP HTTP `8000`, Device/Media/PTZ/Imaging/OSD 호환 응답
- `afterveda-bsp/ptz-kmod`: `/dev/afterveda_ptz` 문자 장치와 GPIO18/19 PWM Pan/Tilt 제어
- 인증: RTSP Digest 인증은 `rail-media --rtsp-user --rtsp-password`, ONVIF UsernameToken은 `afterveda-onvif --username --password`로 선택 적용

## Media/ONVIF 상세 문서

현재 `rail-media`와 `afterveda-onvif`의 기능, 토큰, 요청 흐름, Raspberry Pi 배포 및 ODM 검증 방법은 [docs/media-onvif-runtime/README.md](docs/media-onvif-runtime/README.md)에 정리되어 있다.

## 다음 작업

다음 작업 순서는 [docs/checklist.md](docs/checklist.md)를 기준으로 진행한다.
