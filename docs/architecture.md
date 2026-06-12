# Afterveda 아키텍처

## 개요

Afterveda는 Raspberry Pi 기반 CCTV 장비를 위한 RTSP/ONVIF 애플리케이션과 PTZ BSP를 분리해서 구성한다.

- `afterveda/`: 영상 송출과 ONVIF 서버
- `afterveda-bsp/`: Raspberry Pi GPIO/PWM 같은 보드 의존 제어

## 구성 요소

### 카메라 입력

- Raspberry Pi Camera Module을 기준으로 한다.
- 카메라 인식은 `rpicam-hello --list-cameras`로 확인한다.
- GStreamer 입력은 `libcamerasrc`를 사용한다.

### 미디어 서버

- 실행 파일: `rail-media`
- 역할: PiCam 영상을 H.264로 인코딩하고 RTSP `/live`로 송출
- 제어 API: `http://<pi-ip>:8081`
- 기본 RTSP 주소: `rtsp://<pi-ip>:8554/live`

### ONVIF 서버

- 실행 파일: `afterveda-onvif`
- 역할: WS-Discovery, ONVIF Device/Media/PTZ SOAP 응답
- 영상은 직접 만들지 않고 `rail-media`의 RTSP URI를 반환한다.
- PTZ 요청은 `/dev/afterveda_ptz`에 텍스트 명령으로 기록한다.

### PTZ BSP

- 위치: `afterveda-bsp/ptz-kmod`
- 산출물: `afterveda_ptz.ko`, `afterveda-ptz.dtbo`
- 역할: GPIO18/GPIO19 PWM으로 Pan/Tilt 서보 제어
- 사용자 공간 인터페이스: `/dev/afterveda_ptz`

## 데이터 흐름

```txt
PiCam
→ rail-media
→ GStreamer RTSP 서버
→ rtsp://<pi-ip>:8554/live
→ VLC / VMS / ONVIF 클라이언트
```

## 제어 흐름

```txt
ONVIF 클라이언트 PTZ 명령
→ afterveda-onvif
→ /dev/afterveda_ptz
→ afterveda_ptz 커널 모듈
→ GPIO18/GPIO19 PWM
→ Pan/Tilt 서보
```

## 실행 형태

`rail-media`와 `afterveda-onvif`는 별도 프로세스로 실행한다. `ptz-kmod`는 Raspberry Pi에서 오버레이와 커널 모듈로 로드한다.

```bash
media-server/build/rail-media --profile main

onvif-server/build/afterveda-onvif \
  --xaddr-host <pi-ip> \
  --rtsp-uri rtsp://<pi-ip>:8554/live \
  --rail-control-url http://127.0.0.1:8081 \
  --ptz-device /dev/afterveda_ptz
```
