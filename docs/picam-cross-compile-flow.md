# PiCam 크로스 컴파일 작업 순서

이 문서는 PC에서 Raspberry Pi 4/5 64-bit용 C/C++ 프로그램을 빌드하고, PiCam이 연결된 Raspberry Pi로 보내 실행하는 순서를 정리한다.

## 0. 전체 구조

```txt
PC
→ C/C++ 코드 작성
→ Raspberry Pi 64-bit용 크로스 컴파일
→ 바이너리 전송

Raspberry Pi
→ PiCam 연결
→ libcamera/GStreamer 런타임 설치
→ 바이너리 실행
→ RTSP 스트림 송출
→ VLC에서 확인
```

역할을 분리해서 생각한다.

- PC는 빌드 장비다.
- Raspberry Pi는 실행 장비다.
- PiCam 입력은 Raspberry Pi에서만 확인한다.
- PC에서 GStreamer/libcamera 전체를 새로 빌드하지 않는다.
- Pi에 설치된 GStreamer/libcamera 런타임과 개발 파일을 기준으로 C/C++ 앱만 크로스 컴파일한다.

## 1. Raspberry Pi에서 PiCam 인식 확인

먼저 PC 빌드보다 Raspberry Pi에서 카메라가 정상 인식되는지 확인한다.

```bash
rpicam-hello --list-cameras
```

카메라가 목록에 나오면 preview 테스트를 실행한다.

```bash
rpicam-hello
```

확인할 것:

- [ ] PiCam이 목록에 표시된다
- [ ] preview 또는 테스트 실행이 실패하지 않는다
- [ ] 사용하는 Pi 모델과 OS 버전을 기록한다

기록 예시:

```txt
Pi model: Raspberry Pi 4 / 5
OS: Raspberry Pi OS 64-bit
Camera: Camera Module 2 / 3 / HQ Camera
```

## 2. Raspberry Pi에서 GStreamer/libcamera 확인

PiCam을 GStreamer에서 쓰려면 `libcamerasrc`가 보여야 한다.

```bash
gst-inspect-1.0 libcamerasrc
```

GStreamer 플러그인 전체 상태도 확인한다.

```bash
gst-inspect-1.0 | grep camera
gst-inspect-1.0 | grep h264
```

확인할 것:

- [ ] `libcamerasrc`가 존재한다
- [ ] H.264 인코더가 존재한다
- [ ] 필요한 GStreamer 플러그인이 설치되어 있다

## 3. Raspberry Pi에서 GStreamer 테스트 파이프라인 실행

C/C++ 앱을 만들기 전에 Raspberry Pi에서 명령어 파이프라인을 먼저 검증한다.

Pi 4 계열은 하드웨어 인코더를 우선 검토한다.

```bash
gst-launch-1.0 libcamerasrc \
  ! video/x-raw,width=1280,height=720,framerate=30/1,format=NV12 \
  ! v4l2h264enc extra-controls="controls,repeat_sequence_header=1" \
  ! h264parse \
  ! rtph264pay config-interval=1 pt=96 \
  ! udpsink host=<client-ip> port=5000
```

Pi 5에서는 환경에 따라 소프트웨어 인코더를 먼저 테스트한다.

```bash
gst-launch-1.0 libcamerasrc \
  ! video/x-raw,width=1280,height=720,framerate=30/1,format=NV12 \
  ! x264enc speed-preset=ultrafast tune=zerolatency threads=1 \
  ! h264parse \
  ! rtph264pay config-interval=1 pt=96 \
  ! udpsink host=<client-ip> port=5000
```

이 단계의 목표는 RTSP 서버 완성이 아니라 PiCam 입력과 인코딩이 되는지 확인하는 것이다.

확인할 것:

- [ ] PiCam 입력이 GStreamer로 들어온다
- [ ] H.264 인코딩이 된다
- [ ] 클라이언트에서 영상 수신 테스트가 가능하다

## 4. PC에 크로스 컴파일 환경 준비

PC에는 Raspberry Pi 64-bit용 컴파일러와 빌드 도구를 준비한다.

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

확인할 것:

```bash
aarch64-linux-gnu-g++ --version
cmake --version
pkg-config --version
```

## 5. Pi sysroot를 PC로 복사

PC에서 GStreamer/libcamera 헤더와 라이브러리를 찾으려면 Raspberry Pi의 개발 파일을 sysroot로 복사한다.

PC에서 실행한다.

```bash
mkdir -p sysroot/raspi-aarch64

rsync -avz --delete pi@<pi-ip>:/lib sysroot/raspi-aarch64/
rsync -avz --delete pi@<pi-ip>:/usr/include sysroot/raspi-aarch64/usr/
rsync -avz --delete pi@<pi-ip>:/usr/lib sysroot/raspi-aarch64/usr/
```

Pi의 패키지가 바뀌면 sysroot도 다시 동기화한다.

확인할 것:

- [ ] `sysroot/raspi-aarch64/usr/include/gstreamer-1.0`이 존재한다
- [ ] `sysroot/raspi-aarch64/usr/lib` 아래에 aarch64 라이브러리가 존재한다
- [ ] Pi에 설치된 패키지와 PC sysroot가 같은 상태다

## 6. C/C++ 프로그램 크로스 컴파일

CMake toolchain 파일은 다음 값을 기준으로 만든다.

```txt
CMAKE_SYSTEM_NAME=Linux
CMAKE_SYSTEM_PROCESSOR=aarch64
CMAKE_C_COMPILER=aarch64-linux-gnu-gcc
CMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
CMAKE_SYSROOT=<repo>/sysroot/raspi-aarch64
PKG_CONFIG_SYSROOT_DIR=<repo>/sysroot/raspi-aarch64
```

빌드 흐름은 다음과 같이 잡는다.

```bash
cmake -S . -B build/pi-release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/raspi-aarch64.cmake \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build/pi-release
```

빌드 결과 확인:

```bash
file build/pi-release/<binary-name>
```

확인할 것:

- [ ] 결과물이 `aarch64` Linux 바이너리로 나온다
- [ ] GStreamer 헤더와 라이브러리를 sysroot에서 찾는다
- [ ] PC용 x86_64 바이너리로 잘못 빌드되지 않는다

## 7. 빌드 결과물을 Raspberry Pi로 전송

PC에서 빌드한 바이너리를 Raspberry Pi로 보낸다.

```bash
ssh pi@<pi-ip> "sudo mkdir -p /opt/rail-cctv/bin /etc/rail-cctv"
scp build/pi-release/<binary-name> pi@<pi-ip>:/tmp/
ssh pi@<pi-ip> "sudo mv /tmp/<binary-name> /opt/rail-cctv/bin/ && sudo chmod +x /opt/rail-cctv/bin/<binary-name>"
```

설정 파일이 있으면 같이 보낸다.

```bash
scp config/<config-file> pi@<pi-ip>:/tmp/
ssh pi@<pi-ip> "sudo mv /tmp/<config-file> /etc/rail-cctv/"
```

확인할 것:

- [ ] `/opt/rail-cctv/bin/<binary-name>`이 존재한다
- [ ] 실행 권한이 있다
- [ ] 설정 파일이 `/etc/rail-cctv/`에 있다

## 8. Raspberry Pi에서 실행 테스트

Pi에서 직접 실행한다.

```bash
/opt/rail-cctv/bin/<binary-name>
```

실행 중 문제가 있으면 라이브러리 링크를 확인한다.

```bash
ldd /opt/rail-cctv/bin/<binary-name>
```

확인할 것:

- [ ] 실행 시 shared library not found 오류가 없다
- [ ] PiCam 접근 오류가 없다
- [ ] GStreamer 파이프라인이 시작된다
- [ ] VLC 또는 외부 클라이언트에서 스트림을 확인할 수 있다

## 9. systemd 자동 실행 정리

실행이 확인되면 systemd 서비스로 등록한다.

서비스 파일 위치:

```txt
/etc/systemd/system/rail-media.service
```

서비스 기본 구조:

```ini
[Unit]
Description=Rail CCTV Media Server
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
ExecStart=/opt/rail-cctv/bin/<binary-name>
Restart=always
RestartSec=3
User=pi
Group=video

[Install]
WantedBy=multi-user.target
```

등록과 실행:

```bash
sudo systemctl daemon-reload
sudo systemctl enable rail-media.service
sudo systemctl start rail-media.service
sudo systemctl status rail-media.service
```

로그 확인:

```bash
journalctl -u rail-media.service -f
```

확인할 것:

- [ ] 서비스가 정상 시작된다
- [ ] 재부팅 후 자동 실행된다
- [ ] 로그에서 카메라와 GStreamer 오류를 확인할 수 있다

## 10. 전체 작업 순서 요약

```txt
1. Pi에 PiCam 연결
2. Pi에서 rpicam-hello로 카메라 확인
3. Pi에서 gst-inspect-1.0 libcamerasrc 확인
4. Pi에서 gst-launch-1.0 테스트 파이프라인 실행
5. PC에 aarch64 크로스 컴파일러 설치
6. Pi의 /usr, /lib를 PC sysroot로 복사
7. PC에서 C/C++ 앱 크로스 컴파일
8. file 명령으로 aarch64 바이너리 확인
9. 바이너리를 Pi의 /opt/rail-cctv/bin으로 전송
10. Pi에서 직접 실행 테스트
11. VLC 또는 외부 클라이언트에서 영상 확인
12. systemd 서비스로 자동 실행 등록
```
