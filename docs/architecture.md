# 레일CCTV 하드웨어/미디어 아키텍처

## 개요

이 저장소는 CCTV 하드웨어 제어와 미디어 서버 구성만 다룬다.

## 구성 요소

### Camera Input

- Raspberry Pi Camera Module 또는 USB 카메라를 연결한다.
- 장치 인식 여부를 확인한다.
- 지원 해상도, FPS, 포맷을 확인한다.

### Media Server

- 카메라 입력을 RTSP 스트림으로 송출한다.
- 필요하면 HLS 변환이나 외부 미디어 서버 연동을 검토한다.
- 1차 목표는 VLC 같은 외부 클라이언트에서 RTSP 주소로 영상을 확인하는 것이다.

### Hardware Control

- Pan/Tilt 모터, 릴레이, GPIO/PWM 제어 대상을 정리한다.
- 제어 신호와 전원 구성을 확인한다.
- 좌/우/상/하/정지 같은 기본 제어 동작을 테스트한다.

### Deployment

- 미디어 서버와 하드웨어 제어 프로세스를 systemd 서비스로 등록한다.
- 부팅 후 자동 실행, 재시작 정책, 로그 확인 방법을 문서화한다.

## 데이터 흐름

```txt
CCTV Camera
→ Raspberry Pi / Edge Device
→ Media Server
→ RTSP Stream
→ VLC or External Client
```

## 제어 흐름

```txt
Control Command
→ Hardware Control Process
→ GPIO/PWM or Device Interface
→ Pan/Tilt Motor or Relay
```

## 초기 실행 방식 초안

- 영상 송출은 FFmpeg, GStreamer, 또는 RTSP 미디어 서버 조합을 검토한다.
- 하드웨어 제어는 장비 인터페이스에 맞춰 GPIO/PWM, 시리얼, 또는 제조사 프로토콜 중 하나로 정리한다.
- 자동 실행은 systemd 서비스 파일과 실행 스크립트로 관리한다.
