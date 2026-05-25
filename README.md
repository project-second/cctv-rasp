# Rail CCTV Hardware & Media

이 프로젝트의 범위는 CCTV 하드웨어 제어와 미디어 서버 구성이다.

## 목표

- Raspberry Pi 또는 CCTV 장비에서 카메라 영상 입력 확인
- RTSP 기반 영상 송출 구성
- ONVIF 기반 장비 검색과 RTSP 스트림 연동
- 필요 시 HLS 변환 또는 외부 미디어 서버 연동 구조 정리
- Pan/Tilt 같은 CCTV 하드웨어 제어 방식 정리
- 모터, GPIO/PWM, 릴레이 등 제어 신호 테스트
- systemd 기반 자동 실행 및 로그 확인 방식 정리

## 프로젝트 구조

```txt
rail-cctv/
├── hardware-control/  # CCTV 하드웨어 제어 문서와 테스트 절차
├── media-server/      # 카메라 입력, RTSP 송출, 미디어 서버 구성
├── onvif-server/      # ONVIF discovery, device/media/PTZ service
├── deploy/            # systemd, 실행 스크립트, 배포 문서
├── docs/              # 계획과 아키텍처 문서
└── README.md
```

## 우선순위

```txt
카메라 인식
→ RTSP 송출
→ ONVIF 검색/스트림 URI 제공
→ VLC에서 스트림 확인
→ 하드웨어 제어 신호 테스트
→ PTZ 동작 확인
→ systemd 자동 실행
→ 로그/재시작/장애 대응 정리
```

## 1차 목표

```txt
CCTV Camera
→ Raspberry Pi / Edge Device
→ Media Server
→ RTSP Stream
→ ONVIF Device/Media discovery
→ VLC 또는 외부 클라이언트에서 확인
```

## ONVIF 실행 예시

`rail-media`를 먼저 실행한 뒤 별도 프로세스로 ONVIF 서버를 실행한다.

```bash
onvif-server/build/afterveda-onvif \
  --xaddr-host <pi-ip> \
  --rtsp-uri rtsp://<pi-ip>:8554/live \
  --rail-control-url http://127.0.0.1:8081 \
  --hardware-control hardware-control/build/hardware-control
```

ONVIF 서버는 검색/제어를 담당하고, 실제 영상은 기존 `rail-media` RTSP 스트림을 그대로 사용한다.

## 다음 작업

다음 작업 순서는 [docs/checklist.md](docs/checklist.md)를 기준으로 진행한다.
