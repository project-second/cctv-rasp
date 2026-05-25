# 레일CCTV 하드웨어/미디어 체크리스트

이 문서는 CCTV 하드웨어 제어와 미디어 서버 구성에서 무엇부터 해야 하는지 정리한 실행 체크리스트이다.

## 1. 오늘 바로 할 일

- [x] 프로젝트 범위를 하드웨어 제어와 미디어 서버로 정리한다
- [x] 기존 기본 서버 파일을 제거한다
- [x] `hardware-control/` 디렉터리를 만든다
- [x] `media-server/` 디렉터리를 만든다
- [x] `docs/architecture.md`를 하드웨어/미디어 범위로 다시 작성한다
- [x] 사용할 카메라 종류를 PiCam으로 정한다
- [x] 카메라가 연결될 장비를 Raspberry Pi 4/5 64-bit로 정한다
- [x] 사용할 미디어 서버 방식을 GStreamer 단독으로 정한다
- [x] PiCam 기준 크로스 컴파일 작업 순서 문서를 작성한다
- [x] 제어할 하드웨어 종류를 정리한다

## 2. 1차 목표: 카메라 영상 송출

- [x] PiCam 연결
- [x] 카메라 장치 인식 확인
- [ ] 지원 해상도, FPS, 포맷 확인
- [x] FFmpeg 또는 GStreamer로 로컬 캡처 테스트
- [x] RTSP 송출 방식 결정
- [x] `rail-media` C/C++ 프로젝트 생성
- [x] RTSP 미디어 서버 설계 문서 작성
- [x] RTSP 스트림 송출
- [x] VLC에서 RTSP 주소로 영상 확인
- [x] 성공한 명령어와 설정값 문서화

## 2-1. PC 크로스 컴파일과 Raspberry Pi 배포

- [x] Raspberry Pi에서 `rpicam-hello --list-cameras` 확인
- [x] Raspberry Pi에서 `gst-inspect-1.0 libcamerasrc` 확인
- [x] Raspberry Pi에서 GStreamer 테스트 파이프라인 실행
- [x] Raspberry Pi에서 `gst-inspect-1.0 v4l2h264enc` 또는 `gst-inspect-1.0 x264enc` 확인
- [x] Raspberry Pi에서 `libgstrtspserver-1.0-dev` 설치 확인
- [x] PC에 aarch64 크로스 컴파일러 설치
- [x] Raspberry Pi sysroot를 PC로 복사
- [x] CMake aarch64 toolchain 파일 작성
- [x] `rail-media` 크로스 컴파일 실행 문서 작성
- [x] PC에서 C/C++ 앱 크로스 컴파일
- [x] 빌드 결과물이 aarch64 Linux 바이너리인지 확인
- [x] 바이너리를 Raspberry Pi로 전송
- [x] Raspberry Pi에서 직접 실행 테스트
- [x] VLC 또는 외부 클라이언트에서 스트림 확인

## 3. 2차 목표: 미디어 서버 안정화

- [x] Profile Manager 계획 문서 작성
- [x] Profile Manager 구현
- [x] HTTP API 기반 프로필 조회/변경 구현
- [x] 미디어 서버 후보 비교
- [x] RTSP 포트와 스트림 경로 결정
- [ ] 재시작 시 스트림 자동 복구 방식 정리
- [x] 로그 위치 정리
- [x] 해상도, FPS 기본값 정리
- [x] bitrate 기본값 정리
- [ ] 네트워크 끊김 또는 카메라 미인식 시 대응 절차 정리

## 4. 3차 목표: 하드웨어 제어

- [x] 제어 대상 하드웨어 목록 작성
- [x] 전원 구성과 배선 방식 확인
- [x] GPIO/PWM, 시리얼, 또는 장비 프로토콜 중 제어 방식 결정
- [ ] 좌/우/상/하 제어 테스트
- [ ] 정지 명령 테스트
- [ ] 제어 속도 또는 이동 시간 기본값 정리
- [ ] 안전 정지 조건 정리

## 5. 4차 목표: 자동 실행과 운영 문서

- [ ] 미디어 서버 실행 스크립트 작성
- [ ] 하드웨어 제어 실행 스크립트 작성
- [ ] systemd 서비스 파일 초안 작성
- [ ] 부팅 후 자동 실행 확인
- [ ] 서비스 재시작 정책 정리
- [x] 로그 확인 명령어 정리
- [ ] 장애 발생 시 복구 절차 문서화

## 6. 완료 기준

- [x] 카메라가 장치에서 정상 인식된다
- [x] RTSP 스트림이 외부 클라이언트에서 확인된다
- [x] 별도 ONVIF 서버 모듈을 추가한다
- [x] ONVIF Device/Media SOAP 응답에서 RTSP 주소를 반환한다
- [x] ONVIF PTZ 요청을 hardware-control 명령으로 연결한다
- [ ] 하드웨어 제어 대상이 실제로 동작한다
- [ ] 정지 명령 또는 안전 중단 방법이 확인된다
- [ ] 재부팅 후 필요한 서비스가 자동 실행된다
- [x] 실행 명령어, 포트, 로그 확인 방법이 문서화되어 있다
