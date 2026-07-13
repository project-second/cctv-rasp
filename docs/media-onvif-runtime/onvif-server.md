# afterveda-onvif

`afterveda-onvif`는 `rail-media`를 네트워크 카메라처럼 검색하고 제어할 수 있도록 ONVIF 서비스를 제공한다.

## 네트워크 주소

`--xaddr-host 192.168.0.32`, `--onvif-port 8000` 기준 주소다.

| 기능 | 주소 |
|---|---|
| Device | `http://192.168.0.32:8000/onvif/device_service` |
| Media1 | `http://192.168.0.32:8000/onvif/media_service` |
| Media2 | `http://192.168.0.32:8000/onvif/media2_service` |
| PTZ | `http://192.168.0.32:8000/onvif/ptz_service` |
| Imaging | `http://192.168.0.32:8000/onvif/imaging_service` |
| Health | `http://192.168.0.32:8000/health` |
| Discovery | UDP multicast `239.255.255.250:3702` |

## 실행 옵션

| 옵션 | 의미 | 기본값 |
|---|---|---|
| `--device-name` | ODM에 표시할 장치 이름 | `afterveda-camera` |
| `--manufacturer`, `--model` | 제조사와 모델 | `Afterveda`, `Rail CCTV` |
| `--firmware`, `--serial`, `--hardware-id` | 장치 식별 정보 | 프로젝트 기본값 |
| `--xaddr-host` | Discovery/Capabilities에 광고할 IP 또는 호스트 | `127.0.0.1` |
| `--onvif-port` | SOAP HTTP 포트 | `8000` |
| `--rtsp-uri` | `GetStreamUri`에서 반환할 URI | `rtsp://127.0.0.1:8554/live` |
| `--rail-control-url` | `rail-media` 내부 API | `http://127.0.0.1:8081` |
| `--ptz-device` | PTZ 문자 장치 | `/dev/afterveda_ptz` |
| `--ptz-step` | RelativeMove 최대 이동량 | `5`도 |
| `--ptz-speed` | ContinuousMove 최대 속도 | `30`도/초 |
| `--ptz-dry-run` | 장치에 쓰지 않고 PTZ 로그만 출력 | 미사용 |
| `--username`, `--password` | ONVIF UsernameToken 인증 | 미사용 |

## 실행 예시

```bash
~/afterveda-bin/afterveda-onvif \
  --xaddr-host 192.168.0.32 \
  --rtsp-uri rtsp://192.168.0.32:8554/live \
  --rail-control-url http://127.0.0.1:8081 \
  --ptz-device /dev/afterveda_ptz
```

PTZ 장치 권한이 없으면 임시로 `sudo`를 사용할 수 있지만, 실제 배포에서는 udev 규칙이나 전용 그룹으로 `/dev/afterveda_ptz` 쓰기 권한을 주는 방식이 낫다.

## Device 서비스

지원 작업:

- `GetCapabilities`
- `GetServices`
- `GetServiceCapabilities`
- `GetDeviceInformation`
- `GetSystemDateAndTime`
- `GetScopes`
- `GetHostname`
- `GetNetworkInterfaces`
- `GetUsers`

`GetNetworkInterfaces`는 실행 중인 Linux 시스템의 실제 인터페이스, MAC, IPv4 정보를 조회해 응답한다.

## Media1 서비스

지원 작업:

- `GetServiceCapabilities`
- `GetProfiles`, `GetProfile`
- `GetStreamUri`
- `GetVideoSources`
- `GetVideoSourceConfigurations`, `GetVideoSourceConfiguration`, `GetVideoSourceConfigurationOptions`
- `GetVideoEncoderConfigurations`, `GetVideoEncoderConfiguration`, `GetVideoEncoderConfigurationOptions`
- `SetVideoEncoderConfiguration`
- 표준 Media OSD 작업: `GetOSDs`, `GetOSD`, `GetOSDOptions`, `SetOSD`, `CreateOSD`, `DeleteOSD`

현재 ONVIF Media 객체는 안정적인 `main` 프로필과 `encoder-main` configuration 하나다. 지원 옵션은 다음과 같다.

| 항목 | 지원값 |
|---|---|
| Encoding | H.264 Main |
| Resolution | 640x360, 1280x720, 1920x1080 |
| Frame rate | 30 fps |
| Encoding interval | 1 |
| GOV length | 60 |
| Bitrate | 800..5000 kbps |
| Session timeout | `PT60S` |

`SetVideoEncoderConfiguration`은 `encoder-main`을 새 객체로 교체하지 않고 수정한다. 요청은 `rail-media POST /config`로 전달되고, 이후 Get 요청은 `rail-media GET /profile`의 실제 적용값을 반환한다.

## Media2 서비스

현재 지원 범위는 `GetProfiles`, `GetStreamUri`, `GetServiceCapabilities`다. Media2 OSD는 지원하지 않는 것으로 광고한다. 실질적인 설정 변경과 OSD는 Media1 서비스를 기준으로 사용한다.

## Imaging 서비스

지원 작업:

- `GetServiceCapabilities`
- `GetOptions`
- `GetImagingSettings`
- `SetImagingSettings`

밝기, 대비, 채도만 지원하며 `rail-media /imaging`으로 전달된다. 노출, 게인, 화이트밸런스, 포커스, stabilization은 광고하지 않는다.

## OSD

ONVIF 표준 Media1 `trt` 작업으로 다음 한 개의 OSD를 관리한다.

| 항목 | 값 |
|---|---|
| OSD token | `osd-main` |
| Type | `Text` |
| Text type | `Plain` |
| Position | `UpperLeft` |
| 최대 개수 | 1 |

ODM이 `SetOSD`를 보내면 `rail-media POST /osd`로 전달되어 실제 GStreamer `textoverlay`에 반영된다. ODM 버전에 따라 OSD 편집 UI가 없을 수 있으며, 이 경우 내부 API로 기능을 확인할 수 있다.

## PTZ 서비스

지원 작업:

- `GetServiceCapabilities`
- `GetNodes`, `GetNode`
- `GetConfigurations`, `GetConfiguration`, `GetConfigurationOptions`
- `GetPresets`, `SetPreset`, `GotoPreset`
- `GetStatus`
- `ContinuousMove`, `RelativeMove`, `Stop`
- `SetHomePosition`, `GotoHomePosition`

Zoom 하드웨어가 없으므로 Zoom capability와 상태를 광고하지 않고 Zoom 이동 요청은 fault로 응답한다.

`GetStatus`는 `/dev/afterveda_ptz`에서 다음 값을 읽는다.

```text
pan=90 tilt=45 pan_speed=0 tilt_speed=0
```

`pan_speed` 또는 `tilt_speed`가 0이 아니면 `MOVING`, 모두 0이면 `IDLE`을 반환한다. `ContinuousMove.Timeout`이 지나면 자동으로 `stop` 명령을 기록한다.

## 인증

`--username`과 `--password`를 함께 지정하면 ONVIF UsernameToken 인증을 요구한다. 현재 PasswordText와 PasswordDigest를 확인한다. RTSP Digest 인증은 별도로 `rail-media`에 설정해야 한다.

## Health

```bash
curl -i http://127.0.0.1:8000/health
```

ONVIF 서버는 `rail-media /health`를 함께 조회한다.

- `rail-media` 정상: HTTP `200`, `status: ok`
- `rail-media` 중단 또는 오류: HTTP `503`, `status: degraded`

## 현재 제한

- 공식 ONVIF Device Test Tool 적합성 검증은 아직 완료하지 않았다.
- Event/Notification과 Metadata streaming은 지원하지 않는다.
- Snapshot URI, 오디오, multicast streaming은 지원하지 않는다.
- RTP/RTSP/TCP unicast를 중심으로 지원한다.
- 영상 설정의 재부팅 후 영구 저장은 아직 구현하지 않았다.
