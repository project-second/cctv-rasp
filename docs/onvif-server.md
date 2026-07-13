# ONVIF 서버

`afterveda-onvif`는 기존 `rail-media` RTSP 스트림을 ONVIF 카메라처럼 노출하는 별도 프로세스다. 이 프로세스는 영상을 직접 인코딩하지 않고, ONVIF 검색과 SOAP 요청에 응답한 뒤 `rail-media`의 RTSP URI를 반환한다.

현재 구현은 Device/Media/Media2/PTZ/Imaging 일부 기능을 제공하는 ONVIF 호환 구현이다. Metadata/Event 등 Profile T 필수 기능과 공식 Device Test Tool 검증이 완료되지 않았으므로 Profile T를 광고하지 않는다.

## 런타임 구조

```txt
rail-media
  - PiCam 캡처
  - GStreamer RTSP 스트림
  - HTTP 프로필 제어

afterveda-onvif
  - WS-Discovery UDP 응답
  - ONVIF Device/Media/PTZ SOAP 엔드포인트
  - ONVIF Media2 일부 기능 및 Media1 표준 OSD 응답
  - GetStreamUri에서 rail-media RTSP URI 반환
  - PTZ 요청을 /dev/afterveda_ptz에 기록
```

## 빌드

빌드 환경에는 gSOAP runtime/header와 gSOAP plugin/source 파일이 모두 필요하다. Debian/Raspberry Pi 계열에서는 보통 `gsoap`, `libgsoap-dev` 패키지를 설치한다. 크로스 컴파일 시에는 대상 sysroot에 `/usr/include`, `/usr/lib`, `/usr/share/gsoap`를 함께 동기화해야 한다.

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
  --ptz-device /dev/afterveda_ptz \
  --ptz-speed 30
```

하드웨어 없이 PTZ 흐름만 확인하려면 `--ptz-dry-run`을 추가한다. 실제 서보를 움직이려면 먼저 `../afterveda-bsp/ptz-kmod`의 오버레이와 커널 모듈을 Raspberry Pi에 로드해야 한다.

## 구현 범위

- WS-Discovery: `Probe`, `Resolve` 응답
- Device: `GetCapabilities`, `GetDeviceInformation`, `GetServices`, `GetSystemDateAndTime`, `GetScopes`, `GetHostname`, `GetNetworkInterfaces`, `GetUsers`, `GetServiceCapabilities`
- Media 서비스: `GetProfiles`, `GetProfile`, `GetStreamUri`, `GetVideoSources`, `GetVideoSourceConfiguration*`, `GetVideoEncoderConfiguration*`, `SetVideoEncoderConfiguration`, OSD `GetOSDs`, `GetOSD`, `GetOSDOptions`, `SetOSD`, `CreateOSD`, `DeleteOSD`
- Media 모델: `main` 프로필과 `encoder-main` configuration 토큰을 해상도 변경 후에도 유지하며, `640x360`, `1280x720`, `1920x1080` 중 선택한 실제 `rail-media` 값을 후속 Get 응답에 반환
- Media2 서비스: 기본 H.264 profile, `GetProfiles`, `GetStreamUri`, `GetServiceCapabilities`
- PTZ 서비스: `GetServiceCapabilities`, `GetNodes`, `GetNode`, `GetConfigurations`, `GetConfiguration`, `GetConfigurationOptions`, `GetPresets`, `SetPreset`, `GotoPreset`, `ContinuousMove`, `RelativeMove`, `Stop`, `GetStatus`, `GotoHomePosition`, `SetHomePosition`
- PTZ 상태: `/dev/afterveda_ptz`의 위치와 축별 속도를 읽어 `GetStatus`의 Position 및 `MOVING`/`IDLE`을 실제 상태로 반환
- Imaging 서비스: 밝기/대비/채도 옵션 조회와 설정 변경
- OSD: Media1 `trt` 표준 작업으로 Plain Text/UpperLeft OSD 한 개를 관리하고 `rail-media` overlay에 반영
- WS-Security: `--username`, `--password` 지정 시 UsernameToken `PasswordText`, `PasswordDigest` 확인

## 제약

- Imaging 밝기/대비/채도는 `rail-media` 제어 API를 통해 GStreamer `videobalance`에 연결된다. 노출, 게인, 화이트밸런스는 아직 지원하지 않는다.
- Metadata streaming은 아직 완성되지 않았다. OSD는 Plain Text 한 개와 UpperLeft 위치만 지원한다.
- Zoom 하드웨어가 없으므로 zoom 요청은 SOAP fault로 응답한다.
- PTZ `ContinuousMove`는 ONVIF x/y 값을 `--ptz-speed` 기준 `pan_speed=<deg/s> tilt_speed=<deg/s>` 명령으로 변환하고 요청의 `Timeout` 뒤 자동 정지한다. `Timeout`을 생략하면 `PT1S`를 적용한다. `Stop`은 `stop`, `RelativeMove`와 preset/home 이동은 `pan=<deg> tilt=<deg>` 절대 위치 명령을 사용한다. Zoom 하드웨어는 지원하지 않으며 PTZ 상태에도 Zoom 필드를 광고하지 않는다.
