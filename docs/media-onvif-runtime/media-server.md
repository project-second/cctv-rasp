# rail-media

`rail-media`는 Raspberry Pi 카메라 영상을 생성하고 RTSP로 제공하는 실제 미디어 서버다.

## 영상 파이프라인

```text
libcamerasrc
  → video/x-raw (선택한 해상도, 30fps, NV12)
  → videobalance (밝기/대비/채도)
  → textoverlay (선택적 OSD)
  → v4l2h264enc 또는 x264enc
  → h264parse
  → rtph264pay
  → gst-rtsp-server /live
```

기본 RTSP 주소는 다음과 같다.

```text
rtsp://<PI_IP>:8554/live
```

RTSP factory는 shared 방식이며 실제 파이프라인은 클라이언트가 스트림에 접속할 때 준비된다. 설정 변경 시 factory를 새 설정으로 교체하고 기존 RTSP 클라이언트를 닫는다. 클라이언트는 같은 URI로 다시 연결해 새 설정을 사용한다.

## 실행 옵션

| 옵션 | 의미 | 기본값 |
|---|---|---|
| `--profile <name>` | 시작 화질 프리셋 `low`, `main`, `high` | `main` |
| `--port <port>` | RTSP 포트 | `8554` |
| `--mount <path>` | RTSP mount | `/live` |
| `--control-host <ip>` | HTTP 제어 API bind 주소 | `127.0.0.1` |
| `--control-port <port>` | HTTP 제어 API 포트 | `8081` |
| `--encoder <name>` | `v4l2` 또는 `x264` | `v4l2` |
| `--width`, `--height` | 시작 해상도 직접 지정 | 프리셋 값 |
| `--fps` | 시작 FPS | `30` |
| `--bitrate-kbps` | H.264 bitrate | 프리셋 값 |
| `--osd-text <text>` | 시작 OSD 문자열 | `Afterveda` |
| `--no-osd` | OSD 비활성화 | 비활성화하지 않음 |
| `--rtsp-user`, `--rtsp-password` | RTSP Digest 인증 | 미사용 |
| `--rtsp-realm` | RTSP 인증 realm | `rail-media` |

## 시작 예시

```bash
~/afterveda-bin/rail-media \
  --encoder v4l2 \
  --profile main \
  --control-host 127.0.0.1
```

제어 API는 기본적으로 Pi 내부에서만 접근할 수 있다. 외부 PC에서 API를 직접 호출해야 할 때만 `--control-host 0.0.0.0`을 사용하고 네트워크 접근 제어를 별도로 적용한다.

## HTTP 제어 API

| Method | 경로 | 기능 |
|---|---|---|
| GET | `/health` | RTSP/pipeline/camera/연결 수/오류 상태 |
| GET | `/profile` | 현재 안정적인 `main` 프로필의 encoder 값 |
| GET | `/profiles` | ONVIF에 노출되는 프로필 목록. 현재 한 개 |
| GET | `/presets` | `low/main/high` 화질 프리셋 목록 |
| POST | `/preset/<name>` | 화질 프리셋 적용 |
| POST | `/profile/<name>` | 이전 API 호환용 프리셋 적용 별칭 |
| POST | `/config` | 해상도/FPS/bitrate 직접 변경 |
| GET/POST | `/imaging` | 밝기/대비/채도 조회·변경 |
| GET/POST | `/osd` | OSD 활성 상태와 문자열 조회·변경 |

### 현재 영상 설정

```bash
curl http://127.0.0.1:8081/profile
```

응답 예시:

```json
{
  "profile": "main",
  "width": 1280,
  "height": 720,
  "fps": 30,
  "bitrate_kbps": 2500,
  "encoder": "v4l2"
}
```

### 화질 프리셋

```bash
curl http://127.0.0.1:8081/presets
curl -X POST http://127.0.0.1:8081/preset/low
curl -X POST http://127.0.0.1:8081/preset/main
curl -X POST http://127.0.0.1:8081/preset/high
```

프리셋을 적용해도 ONVIF 프로필 이름은 `main`으로 유지된다. 바뀌는 것은 `encoder-main`의 해상도와 bitrate다.

### 영상 설정 직접 변경

```bash
curl -X POST http://127.0.0.1:8081/config \
  -H 'Content-Type: application/json' \
  -d '{"width":1920,"height":1080,"fps":30,"bitrate_kbps":5000}'
```

동적 변경에서 지원하는 해상도는 `640x360`, `1280x720`, `1920x1080`이고 FPS는 `30`, bitrate는 `800..5000 kbps`다.

### Imaging

```bash
curl http://127.0.0.1:8081/imaging

curl -X POST http://127.0.0.1:8081/imaging \
  -H 'Content-Type: application/json' \
  -d '{"brightness":60,"contrast":55,"color_saturation":50}'
```

값 범위는 `0..100`이며 센서 레지스터가 아니라 GStreamer `videobalance` 후처리에 적용된다.

### OSD

```bash
curl http://127.0.0.1:8081/osd

curl -X POST http://127.0.0.1:8081/osd \
  -H 'Content-Type: application/json' \
  -d '{"enabled":true,"text":"Platform 1"}'

curl -X POST http://127.0.0.1:8081/osd \
  -H 'Content-Type: application/json' \
  -d '{"enabled":false}'
```

현재 OSD는 문자열 한 개, 최대 128자, 좌측 상단 표시만 지원한다.

## Health 상태

```bash
curl -i http://127.0.0.1:8081/health
```

주요 필드:

| 필드 | 의미 |
|---|---|
| `rtsp_server` | RTSP 서버 attach 상태 |
| `pipeline` | `idle`, `preparing`, `ready`, `error` 등 |
| `camera` | `not_checked`, `checking`, `ok`, `error` 등 |
| `resolution`, `fps`, `bitrate_kbps` | 현재 encoder 값 |
| `rtsp_clients` | 연결된 RTSP 클라이언트 수 |
| `started_at`, `uptime_seconds` | 시작 시각과 uptime |
| `last_pipeline_ready_at` | 마지막 준비 완료 시각 |
| `last_error` | 최근 GStreamer 오류 |

파이프라인이 `error`이거나 RTSP 서버가 준비되지 않으면 HTTP `503`을 반환한다. 아직 RTSP 클라이언트가 없어서 pipeline이 `idle`인 것은 오류가 아니다.

## 현재 제한

- RTSP 스트림과 실제 encoder 인스턴스는 하나다.
- 오디오, Snapshot URI, Metadata stream, 녹화는 구현하지 않았다.
- 동적 영상 설정은 프로세스 메모리에 유지되며 현재 재시작 후 자동 복원 파일은 없다.
- 설정 변경 중 기존 RTSP 연결은 종료되므로 ODM/VLC가 다시 연결해야 한다.
- 실제 Pi 카메라가 세 해상도를 모두 정상 협상하는지는 대상 하드웨어에서 확인해야 한다.
