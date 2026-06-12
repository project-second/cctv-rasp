# ONVIF 서버

`afterveda-onvif`는 기존 `rail-media` RTSP 스트림을 ONVIF 카메라처럼 노출하는 별도 프로세스다. 이 프로세스는 영상을 직접 인코딩하지 않고, ONVIF 검색과 SOAP 요청에 응답한 뒤 `rail-media`의 RTSP URI를 반환한다.

현재 구현은 ONVIF Profile T 지향 구현이다. 공식 ONVIF Device Test Tool을 통과하기 전까지 Profile T 인증 구현으로 표현하지 않는다.

## 런타임 구조

```txt
rail-media
  - PiCam 캡처
  - GStreamer RTSP 스트림
  - HTTP 프로필 제어

afterveda-onvif
  - WS-Discovery UDP 응답
  - ONVIF Device/Media/PTZ SOAP 엔드포인트
  - ONVIF Media2/OSD/Profile T 호환 응답
  - GetStreamUri에서 rail-media RTSP URI 반환
  - PTZ 요청을 /dev/afterveda_ptz에 기록
```

## 빌드

```bash
cmake -S onvif-server -B onvif-server/build
cmake --build onvif-server/build
```

## 실행

`rail-media`를 먼저 실행한 뒤 ONVIF 서버를 실행한다.

```bash
media-server/build/rail-media --profile main

onvif-server/build/afterveda-onvif \
  --device-name afterveda-camera \
  --xaddr-host <pi-ip> \
  --rtsp-uri rtsp://<pi-ip>:8554/live \
  --rail-control-url http://127.0.0.1:8081 \
  --ptz-device /dev/afterveda_ptz
```

하드웨어 없이 PTZ 흐름만 확인하려면 `--ptz-dry-run`을 추가한다. 실제 서보를 움직이려면 먼저 `../afterveda-bsp/ptz-kmod`의 오버레이와 커널 모듈을 Raspberry Pi에 로드해야 한다.

## 구현 범위

- WS-Discovery: `Probe`, `Resolve` 응답
- Device: `GetCapabilities`, `GetDeviceInformation`, `GetServices`, `GetSystemDateAndTime`, `GetScopes`, `GetHostname`, `GetNetworkInterfaces`, `GetUsers`, `GetServiceCapabilities`
- Media 서비스: `GetProfiles`, `GetStreamUri`, `GetVideoEncoderConfiguration`, `SetVideoEncoderConfiguration`
- Media2 서비스: 기본 H.264 profile, `GetProfiles`, `GetStreamUri`, `GetServiceCapabilities`
- PTZ 서비스: `GetNodes`, `GetConfigurations`, `GetPresets`, `SetPreset`, `GotoPreset`, `ContinuousMove`, `RelativeMove`, `Stop`, `GetStatus`, `GotoHomePosition`, `SetHomePosition`
- Imaging 서비스: 기본 옵션/설정 호환 응답
- Events 서비스: 기본 event properties와 PullPoint 호환 응답
- OSD 서비스: 기본 텍스트 OSD 호환 응답
- WS-Security: `--username`, `--password` 지정 시 UsernameToken `PasswordText`, `PasswordDigest` 확인

## 제약

- Imaging 설정은 현재 호환 응답 중심이며 실제 센서 제어로 연결되어 있지 않다.
- Metadata streaming과 실제 영상 OSD 오버레이는 아직 완성되지 않았다.
- Zoom 하드웨어가 없으므로 zoom 요청은 SOAP fault로 응답한다.
- PTZ는 속도 제어가 아니라 방향별 step 명령으로 처리한다.
