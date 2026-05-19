# Media Server

카메라 입력과 RTSP 영상 송출을 정리하는 영역이다.

## 담당 범위

- 카메라 장치 인식
- FFmpeg 또는 GStreamer 기반 캡처 테스트
- RTSP 송출 구성
- VLC 또는 외부 클라이언트 확인
- 해상도, FPS, bitrate 설정 정리

## 첫 작업

- 사용할 카메라 종류 확인 완료: PiCam
- 장치 인식과 GStreamer 입력 확인 완료
- RTSP 송출 방식 결정: GStreamer + gst-rtsp-server
- 성공한 스트리밍 명령어 문서화

## 관련 문서

- [RTSP 미디어 서버 설계](../docs/media-server-design.md)
- [Profile Manager 계획](../docs/profile-manager-plan.md)
- [PiCam 크로스 컴파일 작업 순서](../docs/picam-cross-compile-flow.md)

## 크로스 컴파일

PC/WSL에서 Raspberry Pi 4/5 64-bit용으로 빌드한다.

```bash
cmake -S media-server -B media-server/build/pi-release \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/raspi-aarch64.cmake" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build media-server/build/pi-release
```

자세한 순서는 [rail-media 크로스 컴파일 실행 문서](../docs/cross-compile-rail-media.md)를 따른다.

## Raspberry Pi 실행

```bash
/opt/rail-cctv/bin/rail-media
```

기본 프로필은 `main`이며 720p/30fps로 시작한다.

```bash
/opt/rail-cctv/bin/rail-media --profile low
/opt/rail-cctv/bin/rail-media --profile main
/opt/rail-cctv/bin/rail-media --profile high
```

VLC 접속 주소:

```txt
rtsp://<pi-ip>:8554/live
```

프로필 조회와 변경:

```bash
curl http://<pi-ip>:8081/profile
curl http://<pi-ip>:8081/profiles
curl -X POST http://<pi-ip>:8081/profile/low
curl -X POST http://<pi-ip>:8081/profile/main
curl -X POST http://<pi-ip>:8081/profile/high
```
