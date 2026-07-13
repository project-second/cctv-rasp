# Afterveda Media/ONVIF 실행 구조

이 폴더는 `rail-media`와 `afterveda-onvif`가 Raspberry Pi CCTV에서 어떤 역할을 맡고, 서로 어떻게 연결되는지 현재 소스 코드 기준으로 설명한다.

## 문서 구성

- [media-server.md](media-server.md): 카메라 입력, GStreamer/RTSP, 화질 변경, OSD, Imaging, HTTP 제어 API
- [onvif-server.md](onvif-server.md): WS-Discovery, Device, Media, PTZ, Imaging, OSD와 인증
- [integration-flow.md](integration-flow.md): ODM 요청이 실제 영상·PTZ 하드웨어에 반영되는 과정
- [deploy-and-test.md](deploy-and-test.md): Raspberry Pi 배포, 실행, 상태 확인과 기능별 검증 명령

## 전체 구조

```text
ONVIF Device Manager / VMS
        │
        ├─ WS-Discovery UDP 3702
        ├─ ONVIF SOAP HTTP 8000
        │        │
        │        ▼
        │   afterveda-onvif
        │        ├─ HTTP control API 호출 ──────┐
        │        └─ /dev/afterveda_ptz 제어      │
        │                                       ▼
        └─ RTSP 8554/live ──────────────── rail-media
                                                │
                                                ├─ libcamerasrc
                                                ├─ videobalance
                                                ├─ textoverlay
                                                ├─ H.264 encoder
                                                └─ gst-rtsp-server
```

## 프로세스 역할

| 프로세스 | 담당 기능 | 기본 포트 |
|---|---|---:|
| `rail-media` | Pi 카메라 캡처, 영상 처리, H.264 인코딩, RTSP 송출, 미디어 설정 적용 | RTSP `8554`, 제어 API `127.0.0.1:8081` |
| `afterveda-onvif` | 장치 검색, ONVIF SOAP 응답, 미디어 설정 중계, PTZ 장치 제어 | HTTP `8000`, WS-Discovery UDP `3702` |

`afterveda-onvif`는 영상을 직접 만들지 않는다. `GetStreamUri` 요청에는 `rail-media`의 RTSP 주소를 반환하고, 화질·Imaging·OSD 변경은 내부 HTTP API로 전달한다.

## 현재 미디어 모델

실제 하드웨어가 카메라 하나와 인코더 파이프라인 하나이므로 ONVIF에는 다음 객체를 안정적으로 노출한다.

| 객체 | 토큰 | 의미 |
|---|---|---|
| Media Profile | `main` | ODM에서 선택하는 미디어 프로필 |
| Video Source | `picam-source` | Raspberry Pi 카메라 입력 |
| Video Source Configuration | `video-source` | 카메라 입력 영역 설정 |
| Video Encoder Configuration | `encoder-main` | 실제 H.264 해상도/FPS/bitrate 설정 |
| PTZ Configuration | `ptz` | Pan/Tilt 설정 |
| PTZ Node | `ptz-node` | Pan/Tilt 하드웨어 노드 |
| OSD | `osd-main` | 좌측 상단 Plain Text OSD |

해상도가 바뀌어도 `main`과 `encoder-main` 토큰은 바뀌지 않는다. 따라서 ODM에서 설정한 뒤 다시 조회하면 같은 configuration에서 변경된 값이 반환된다.

## 지원 화질

| 해상도 | FPS | 대표 bitrate 프리셋 |
|---:|---:|---:|
| 640x360 | 30 | 800 kbps |
| 1280x720 | 30 | 2500 kbps |
| 1920x1080 | 30 | 5000 kbps |

ONVIF 설정에서 bitrate는 `800..5000 kbps`, FPS는 현재 `30`만 지원한다. `low`, `main`, `high`는 독립 ONVIF 프로필이 아니라 위 값을 빠르게 적용하는 `rail-media` 로컬 프리셋이다.

## 표준 구현 범위에 대한 표현

이 프로젝트는 ONVIF Device/Media/Media2/PTZ/Imaging의 일부 기능을 구현하지만 공식 ONVIF 적합성 인증을 받은 제품은 아니다. Metadata/Event와 공식 Device Test Tool 검증이 완료되지 않았으므로 Profile T scope를 광고하지 않는다.

참고 규격:

- [ONVIF Media Service Specification](https://www.onvif.org/specs/srv/media/ONVIF-Media-Service-Spec.pdf)
- [ONVIF PTZ Service Specification](https://www.onvif.org/onvif/specs/srv/ptz/ONVIF-PTZ-Service-Spec.pdf)
