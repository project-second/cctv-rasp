# 배포와 검증

아래 예시는 Raspberry Pi 계정이 `seok`, IP가 `192.168.0.32`, 실행파일 디렉터리가 `~/afterveda-bin`인 경우다.

## Raspberry Pi용 빌드

개발 PC에서:

```bash
cd ~/workspace/afterveda
cmake --preset pi-release
cmake --build --preset pi-release -j2
```

산출물:

```text
build/pi-release/media-server/rail-media
build/pi-release/onvif-server/afterveda-onvif
```

## 전송

```bash
cd ~/workspace/afterveda

scp \
  build/pi-release/media-server/rail-media \
  build/pi-release/onvif-server/afterveda-onvif \
  seok@192.168.0.32:~/afterveda-bin/
```

Pi에서:

```bash
chmod +x ~/afterveda-bin/rail-media
chmod +x ~/afterveda-bin/afterveda-onvif
```

## 실행 순서

먼저 `rail-media`를 실행한다.

```bash
~/afterveda-bin/rail-media \
  --encoder v4l2 \
  --profile main \
  --control-host 127.0.0.1
```

다른 터미널에서 `afterveda-onvif`를 실행한다.

```bash
~/afterveda-bin/afterveda-onvif \
  --xaddr-host 192.168.0.32 \
  --rtsp-uri rtsp://192.168.0.32:8554/live \
  --rail-control-url http://127.0.0.1:8081 \
  --ptz-device /dev/afterveda_ptz
```

PTZ 권한 문제를 분리해서 확인하려면 먼저 다음처럼 실행할 수 있다.

```bash
~/afterveda-bin/afterveda-onvif \
  --xaddr-host 192.168.0.32 \
  --rtsp-uri rtsp://192.168.0.32:8554/live \
  --rail-control-url http://127.0.0.1:8081 \
  --ptz-dry-run
```

## 포트 충돌 확인

`failed to bind ... port: 98`은 보통 기존 프로세스가 포트를 사용 중이라는 의미다.

```bash
sudo ss -ltnp | grep -E ':8000|:8081|:8554'
sudo ss -lunp | grep ':3702'
```

실행 중인 프로세스를 확인한 뒤 기존 인스턴스를 정상 종료하고 새 바이너리를 실행한다.

## 기본 상태 확인

```bash
curl -i http://127.0.0.1:8081/health
curl -i http://127.0.0.1:8000/health
curl http://127.0.0.1:8081/profile
curl http://127.0.0.1:8081/presets
```

RTSP 재생:

```bash
ffplay rtsp://192.168.0.32:8554/live
```

또는 VLC에서 같은 주소를 연다.

## 화질 변경 검증

고화질 적용:

```bash
curl -X POST http://127.0.0.1:8081/preset/high
curl http://127.0.0.1:8081/profile
```

기대값:

```text
width=1920, height=1080, fps=30, bitrate_kbps=5000
```

저화질 적용:

```bash
curl -X POST http://127.0.0.1:8081/preset/low
curl http://127.0.0.1:8081/profile
```

기대값:

```text
width=640, height=360, fps=30, bitrate_kbps=800
```

ODM에서는 장치를 삭제 후 다시 검색하거나 Refresh를 눌러 이전 `low/high` ONVIF 프로필 캐시를 제거한다. 새 구조에서는 `main` 프로필 하나가 보이고 Video streaming 화면의 해상도 목록에서 세 해상도를 선택한다.

## OSD 검증

```bash
curl -X POST http://127.0.0.1:8081/osd \
  -H 'Content-Type: application/json' \
  -d '{"enabled":true,"text":"Platform 1"}'

curl http://127.0.0.1:8081/osd
```

RTSP 클라이언트가 재연결된 뒤 영상 좌측 상단에 글자가 표시되어야 한다.

## Imaging 검증

```bash
curl -X POST http://127.0.0.1:8081/imaging \
  -H 'Content-Type: application/json' \
  -d '{"brightness":65,"contrast":55,"color_saturation":50}'

curl http://127.0.0.1:8081/imaging
```

## PTZ 검증

커널 장치 상태:

```bash
cat /dev/afterveda_ptz
```

예상 형식:

```text
pan=90 tilt=45 pan_speed=0 tilt_speed=0
```

직접 연속 이동 후 정지:

```bash
echo 'pan_speed=0 tilt_speed=10' | sudo tee /dev/afterveda_ptz
cat /dev/afterveda_ptz
echo 'stop' | sudo tee /dev/afterveda_ptz
```

현재 각도 범위는 Pan `30..150`, Tilt `0..160`, 기본 위치는 `pan=90 tilt=45`다.

## ODM 확인 순서

1. 기존 Afterveda 장치가 이전 프로필 정보를 캐시했다면 ODM에서 제거한다.
2. Discovery로 장치를 다시 찾는다.
3. `Profiles`에서 `main` 하나가 보이는지 확인한다.
4. `Video streaming`에서 1920x1080을 적용하고 영상이 재연결되는지 확인한다.
5. 다시 640x360을 적용한다.
6. Pi에서 `curl http://127.0.0.1:8081/profile`로 실제값을 비교한다.
7. PTZ control에서 이동 후 `GetStatus`가 MOVING/IDLE로 바뀌는지 확인한다.

## 배포 후 꼭 확인할 제한

- Raspberry Pi 카메라가 640x360, 1280x720, 1920x1080을 모두 협상하는지 확인한다.
- 설정 변경 후 ODM이 RTSP에 자동 재연결하는지 확인한다.
- `rail-media`를 재시작하면 동적 설정이 시작 옵션/프리셋 값으로 돌아가는 점을 확인한다.
- `afterveda-onvif /health`가 `rail-media` 중단 시 HTTP 503을 반환하는지 확인한다.
- 공식 ONVIF 인증을 주장하기 전 Device Test Tool 검증이 별도로 필요하다.
