# Media/ONVIF 연동 흐름

## 장치 검색과 영상 재생

```text
1. ODM → WS-Discovery Probe
2. afterveda-onvif → device_service XAddr 응답
3. ODM → GetCapabilities / GetServices / GetProfiles
4. afterveda-onvif → main 프로필과 encoder-main 응답
5. ODM → GetStreamUri(ProfileToken=main)
6. afterveda-onvif → rtsp://<PI_IP>:8554/live 응답
7. ODM → rail-media RTSP 연결
8. rail-media → 카메라 pipeline 준비 후 H.264 송출
```

`afterveda-onvif`는 SOAP 제어 경로에 있고 영상 데이터 경로에는 없다. 실제 영상은 ODM과 `rail-media` 사이에서 RTSP로 전송된다.

## ODM 화질 변경

```text
ODM
  → GetVideoEncoderConfigurationOptions
  ← 640x360 / 1280x720 / 1920x1080, 30fps, 800..5000kbps

ODM
  → SetVideoEncoderConfiguration(token=encoder-main)
  → afterveda-onvif
  → POST rail-media:8081/config
  → 새 RTSP factory 설치
  → 기존 RTSP client 종료
  → ODM 재연결

ODM
  → GetVideoEncoderConfiguration(encoder-main)
  ← rail-media에 실제 적용된 새 값
```

중요한 점은 해상도가 바뀌어도 토큰이 `encoder-main`으로 유지된다는 것이다. `custom`, `encoder-low`, `encoder-high` 같은 임시 객체로 바뀌지 않으므로 ODM 화면과 실제 pipeline 상태가 일치한다.

## OSD 변경

```text
ODM SetOSD(osd-main, PlainText)
  → afterveda-onvif Media1 OSD handler
  → GET rail-media /osd
  → POST rail-media /osd
  → GStreamer textoverlay 설정으로 factory 재생성
  → RTSP 재연결 후 새 글자 표시
```

## Imaging 변경

```text
ODM SetImagingSettings
  → afterveda-onvif Imaging handler
  → POST rail-media /imaging
  → brightness/contrast/color_saturation 갱신
  → GStreamer videobalance 설정으로 factory 재생성
```

현재 Imaging은 카메라 센서 제어가 아니라 영상 후처리다.

## PTZ 이동

```text
ODM ContinuousMove(x, y, Timeout)
  → afterveda-onvif PTZ handler
  → x/y를 pan_speed/tilt_speed로 변환
  → /dev/afterveda_ptz write
  → 커널 모듈 worker가 PWM 갱신
  → Timeout 후 stop 자동 기록
```

RelativeMove는 현재 위치를 읽고 증분 각도를 계산한 뒤 `pan=<deg> tilt=<deg>` 절대 위치 명령으로 기록한다.

## 상태 조회

```text
GET :8000/health
  → afterveda-onvif 자체 HTTP 응답
  → GET :8081/health
  → rail-media RTSP/pipeline/camera/client/error 포함
  → 정상 200, 미디어 오류 또는 연결 실패 503
```

PTZ 상태는 별도 경로다.

```text
ODM GetStatus
  → cat과 같은 방식으로 /dev/afterveda_ptz read
  → position + pan_speed/tilt_speed
  → ONVIF Position + MOVING/IDLE 응답
```

## 설정 변경 시 연결이 잠깐 끊기는 이유

해상도, Imaging, OSD는 GStreamer launch pipeline 구성 요소다. 현재 구현은 안전하게 새 설정을 적용하기 위해 RTSP factory를 교체하고 기존 클라이언트를 닫는다. 따라서 변경 직후 영상이 잠깐 끊기는 것은 정상이며 ODM/VLC가 같은 URI로 다시 연결해야 한다.
