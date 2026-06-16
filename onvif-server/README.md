# Afterveda ONVIF 서버

`afterveda-onvif`는 기존 `rail-media` RTSP 스트림을 ONVIF 카메라처럼 노출한다.

이 프로세스는 영상을 인코딩하지 않는다. ONVIF discovery와 SOAP 요청에 응답하고, 영상 URI 요청에는 `rail-media`가 제공하는 RTSP 주소를 반환한다.

현재 구현은 ONVIF Profile T 지향 구현이지만 공식 Profile T 인증 구현은 아니다. 공식 ONVIF Device Test Tool을 통과하기 전까지는 Profile T 작업 중인 구현으로 본다.

## 빌드

```bash
cmake -S onvif-server -B onvif-server/build
cmake --build onvif-server/build
```

## 소스 구조

```txt
src/
  main.cpp      실행 진입점
  core/         앱 실행 흐름, 설정, 공통 타입/유틸리티
  network/      HTTP, WS-Discovery, rail-media 제어 API 클라이언트
  media/        ONVIF Media 모델과 XML 직렬화
  ptz/          PTZ 장치 접근과 PTZ 서비스 로직
  soap/         gSOAP ONVIF 서비스 핸들러
```

## 실행

```bash
onvif-server/build/afterveda-onvif \
  --device-name afterveda-camera \
  --xaddr-host 192.168.0.32 \
  --rtsp-uri rtsp://192.168.0.32:8554/live \
  --ptz-device /dev/afterveda_ptz
```

## 주요 옵션

```txt
--onvif-port <port>          ONVIF HTTP/SOAP 포트. 기본값: 8000
--xaddr-host <ip-or-host>    ONVIF 서비스 URL에 노출할 호스트
--rtsp-uri <uri>             GetStreamUri에서 반환할 RTSP URI
--rail-control-url <url>     rail-media 제어 API 주소. 기본값: http://127.0.0.1:8081
--ptz-device <path>          PTZ 문자 장치. 기본값: /dev/afterveda_ptz
--ptz-step <deg>             ONVIF x/y 1.0 요청에서 이동할 최대 각도. 기본값: 5
--ptz-dry-run                PTZ 장치에 쓰지 않고 로그만 출력
--username <user>            ONVIF UsernameToken 사용자명 요구
--password <password>        ONVIF UsernameToken 비밀번호 요구
```

## 지원 범위

- WS-Discovery: `Probe`, `Resolve` 응답
- Device: `GetCapabilities`, `GetDeviceInformation`, `GetServices`, `GetSystemDateAndTime`, `GetScopes`, `GetHostname`, `GetNetworkInterfaces`, `GetUsers`, `GetServiceCapabilities`
- Media 서비스: `GetProfiles`, `GetStreamUri`, `GetVideoEncoderConfiguration`, `SetVideoEncoderConfiguration`
- Media2 서비스: 기본 H.264 profile, `GetProfiles`, `GetStreamUri`, `GetServiceCapabilities`
- PTZ 서비스: `GetNodes`, `GetConfigurations`, `GetPresets`, `SetPreset`, `GotoPreset`, `ContinuousMove`, `RelativeMove`, `Stop`, `GetStatus`, `GotoHomePosition`, `SetHomePosition`
- Imaging 서비스: 기본 `GetOptions`, `GetImagingSettings`, `SetImagingSettings` 호환 응답
- Events 서비스: 기본 `GetEventProperties`, `CreatePullPointSubscription`, `PullMessages`, `Renew`, `Unsubscribe` 호환 응답
- OSD 서비스: 기본 텍스트 OSD 조회/수정 호환 응답
- WS-Security: `--username`, `--password` 지정 시 UsernameToken `PasswordText`, `PasswordDigest` 확인

PTZ 요청은 Afterveda PTZ 커널 모듈이 제공하는 `/dev/afterveda_ptz`로 전달한다. 서버는 현재 위치를 `cat /dev/afterveda_ptz`와 같은 방식으로 읽고, ONVIF `ContinuousMove`/`RelativeMove`의 x/y 값을 새 절대 각도로 변환해서 다음 형식으로 기록한다.

```bash
echo "pan=120 tilt=80" | sudo tee /dev/afterveda_ptz
```

`GotoPreset`, `SetHomePosition`, `GotoHomePosition`은 기본 위치인 `pan=90 tilt=45`로 이동한다. Zoom 하드웨어는 아직 없으므로 zoom 요청은 SOAP fault로 응답한다.
