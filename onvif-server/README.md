# Afterveda ONVIF 서버

`afterveda-onvif`는 기존 `rail-media` RTSP 스트림을 ONVIF 카메라처럼 노출한다.

이 프로세스는 영상을 인코딩하지 않는다. ONVIF discovery와 SOAP 요청에 응답하고, 영상 URI 요청에는 `rail-media`가 제공하는 RTSP 주소를 반환한다.

현재 구현은 Device/Media/Media2/PTZ/Imaging 일부 기능을 제공하는 ONVIF 호환 구현이다. Metadata/Event 등 Profile T 필수 기능과 공식 Device Test Tool 검증이 완료되지 않았으므로 Profile T를 광고하지 않는다.

## 빌드

`afterveda-onvif` 빌드에는 gSOAP runtime/header와 plugin/source 파일이 필요하다. Debian/Raspberry Pi 계열에서는 `gsoap`, `libgsoap-dev` 패키지를 설치한다. 크로스 컴파일 시에는 대상 sysroot에 `/usr/include`, `/usr/lib`, `/usr/share/gsoap`를 같이 동기화한다.

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

## 상태 확인

```bash
curl -i http://127.0.0.1:8000/health
```

ONVIF `/health`는 내부 `rail-media`의 `GET /health`를 함께 조회한다. 미디어 서버가 정상이면 HTTP `200`과 `status: ok`, 미디어 서버가 중단됐거나 오류 상태이면 HTTP `503`과 `status: degraded`를 반환한다. 응답의 `rail_media.health`에는 pipeline, 카메라, RTSP client 수, uptime, 최근 준비 시각과 최근 GStreamer 오류가 포함된다.

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
- Media 서비스: `GetProfiles`, `GetProfile`, `GetStreamUri`, `GetVideoSources`, `GetVideoSourceConfiguration*`, `GetVideoEncoderConfiguration*`, `SetVideoEncoderConfiguration`, 표준 OSD `GetOSDs`, `GetOSD`, `GetOSDOptions`, `SetOSD`, `CreateOSD`, `DeleteOSD`
- Media 모델: 실제 인코더 인스턴스에 맞춰 안정적인 `main` 프로필/`encoder-main` 설정 하나를 제공하며, H.264 해상도 `640x360`, `1280x720`, `1920x1080`을 선택 가능. Set 이후 Get은 `rail-media`에 적용된 동일한 값을 반환
- Media2 서비스: 기본 H.264 profile, `GetProfiles`, `GetStreamUri`, `GetServiceCapabilities`
- PTZ 서비스: `GetServiceCapabilities`, `GetNodes`, `GetNode`, `GetConfigurations`, `GetConfiguration`, `GetConfigurationOptions`, `GetPresets`, `SetPreset`, `GotoPreset`, `ContinuousMove`, `RelativeMove`, `Stop`, `GetStatus`, `GotoHomePosition`, `SetHomePosition`
- PTZ 상태: 커널 PTZ 장치의 `pan`, `tilt`, `pan_speed`, `tilt_speed`를 읽어 현재 위치와 `MOVING`/`IDLE`을 반환
- Imaging 서비스: 밝기/대비/채도 `GetOptions`, `GetImagingSettings`, `SetImagingSettings`
- OSD: Media 서비스의 `trt` 작업으로 Plain Text/UpperLeft OSD 한 개를 조회·생성·수정·삭제하고 `rail-media`의 실제 영상 overlay에 반영
- WS-Security: `--username`, `--password` 지정 시 UsernameToken `PasswordText`, `PasswordDigest` 확인

PTZ 요청은 Afterveda PTZ 커널 모듈이 제공하는 `/dev/afterveda_ptz`로 전달한다. 서버는 현재 위치를 `cat /dev/afterveda_ptz`와 같은 방식으로 읽고, ONVIF `ContinuousMove`/`RelativeMove`의 x/y 값을 새 절대 각도로 변환해서 다음 형식으로 기록한다. `ContinuousMove`는 요청의 `Timeout` 뒤 자동으로 정지하며, 생략하면 기본값 `PT1S`를 적용한다. Zoom 하드웨어는 지원하지 않는다.

```bash
echo "pan=120 tilt=80" | sudo tee /dev/afterveda_ptz
```

`GotoPreset`, `SetHomePosition`, `GotoHomePosition`은 기본 위치인 `pan=90 tilt=45`로 이동한다. Zoom 하드웨어는 아직 없으므로 zoom 요청은 SOAP fault로 응답한다.
