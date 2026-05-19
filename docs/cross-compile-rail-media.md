# rail-media 크로스 컴파일 실행 문서

이 문서는 PC/WSL에서 `rail-media`를 Raspberry Pi 4/5 64-bit용으로 크로스 컴파일하는 실제 명령 순서를 정리한다.

## 1. 전제

- 대상 장비: Raspberry Pi 4/5 64-bit
- 카메라: PiCam
- 구현: C/C++
- 미디어: GStreamer + `gst-rtsp-server`
- PC 빌드 결과물: `media-server/build/pi-release/rail-media`

## 2. Raspberry Pi에 개발 패키지 설치

Pi에서 먼저 실행한다.

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  pkg-config \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev \
  libgstrtspserver-1.0-dev \
  gstreamer1.0-tools \
  gstreamer1.0-plugins-base \
  gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad \
  gstreamer1.0-plugins-ugly
```

Pi에서 확인한다.

```bash
gst-inspect-1.0 libcamerasrc
gst-inspect-1.0 rtph264pay
gst-inspect-1.0 v4l2h264enc
gst-inspect-1.0 x264enc
```

## 3. PC/WSL에 크로스 컴파일 도구 설치

PC/WSL에서 실행한다.

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  pkg-config \
  gcc-aarch64-linux-gnu \
  g++-aarch64-linux-gnu \
  rsync
```

확인한다.

```bash
aarch64-linux-gnu-g++ --version
cmake --version
pkg-config --version
```

## 4. Pi sysroot 동기화

PC/WSL에서 실행한다.

```bash
mkdir -p sysroot/raspi-aarch64/usr

rsync -avz --delete pi@<pi-ip>:/lib sysroot/raspi-aarch64/
rsync -avz --delete pi@<pi-ip>:/usr/include sysroot/raspi-aarch64/usr/
rsync -avz --delete pi@<pi-ip>:/usr/lib sysroot/raspi-aarch64/usr/
```

확인한다.

```bash
test -d sysroot/raspi-aarch64/usr/include/gstreamer-1.0
test -d sysroot/raspi-aarch64/usr/lib/aarch64-linux-gnu/pkgconfig
```

## 5. PC/WSL에서 크로스 컴파일

저장소 루트에서 실행한다.

```bash
cmake -S media-server -B media-server/build/pi-release \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/raspi-aarch64.cmake" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build media-server/build/pi-release
```

결과 확인:

```bash
file media-server/build/pi-release/rail-media
```

정상이라면 `aarch64` 또는 `ARM aarch64` Linux 실행 파일로 표시되어야 한다.

빌드 중 `time_t`, `__time64_t` 관련 에러가 나면 sysroot의 아키텍처 헤더 경로가 빠진 것이다.
`cmake/toolchains/raspi-aarch64.cmake`에서 다음 경로가 포함되어 있어야 한다.

```txt
sysroot/raspi-aarch64/usr/include/aarch64-linux-gnu
sysroot/raspi-aarch64/usr/lib/aarch64-linux-gnu
sysroot/raspi-aarch64/lib/aarch64-linux-gnu
```

## 6. Raspberry Pi로 전송

PC/WSL에서 실행한다.

```bash
ssh pi@<pi-ip> "sudo mkdir -p /opt/rail-cctv/bin"
scp media-server/build/pi-release/rail-media pi@<pi-ip>:/tmp/rail-media
ssh pi@<pi-ip> "sudo mv /tmp/rail-media /opt/rail-cctv/bin/rail-media && sudo chmod +x /opt/rail-cctv/bin/rail-media"
```

## 7. Raspberry Pi에서 실행

Pi에서 실행한다.

```bash
/opt/rail-cctv/bin/rail-media
```

Pi 5 또는 `v4l2h264enc`가 실패하는 환경에서는 다음처럼 소프트웨어 인코더로 실행한다.

```bash
/opt/rail-cctv/bin/rail-media --encoder x264
```

포트 확인:

```bash
ss -lntp | grep 8554
```

VLC에서 접속:

```txt
rtsp://<pi-ip>:8554/live
```
