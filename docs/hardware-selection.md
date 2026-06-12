# 하드웨어 선정

이 문서는 레일CCTV 하드웨어 제어의 1차 대상을 정리한다.

## 1차 목표

1차 하드웨어 제어 목표는 카메라 방향을 움직이는 Pan/Tilt 제어다.

```txt
Raspberry Pi
→ Linux PWM framework
→ GPIO18/GPIO19 PWM
→ Pan Servo
→ Tilt Servo
```

레일 이동은 1차 범위에서 제외하고, Pan/Tilt 방향 제어가 안정화된 뒤 별도 단계에서 검토한다.

## 추천 구성

| 구성품 | 용도 | 비고 |
|---|---|---|
| Pan/Tilt 브래킷 | 카메라를 좌우/상하로 움직이는 기구부 | PiCam 고정 가능 여부 확인 |
| 서보 모터 2개 | Pan 축과 Tilt 축 구동 | 브래킷 무게와 카메라 무게를 버틸 토크 필요 |
| 외부 5V 전원 | 서보 전원 공급 | Raspberry Pi 5V 핀에 서보 전류를 직접 의존하지 않는다 |
| 공통 GND | 제어 기준 전압 공유 | Raspberry Pi GND와 서보 전원 GND를 연결 |

선택 구성:

- 전원 스위치
- 퓨즈
- 물리 정지 구조
- 케이블 고정 부품

## 후보 비교

| 후보 | 장점 | 단점 | 판단 |
|---|---|---|---|
| 2축 서보 + Raspberry Pi PWM | BSP 커널 모듈로 ONVIF PTZ와 직접 연결된다. 각도 기반 제어가 쉽다. | GPIO18/GPIO19 PWM 오버레이와 커널 모듈 로드가 필요하다. | 1차 추천 |
| DC 모터 + H-Bridge | 레일 이동이나 연속 회전에 적합하다. | Pan/Tilt 각도 제어에는 별도 위치 센서가 필요하다. | 레일 이동용 후보 |
| 스텝 모터 + 드라이버 | 정밀 위치 제어에 유리하다. | 전원, 드라이버, 제어가 서보보다 복잡하다. | 정밀 이동용 후보 |
| 상용 PTZ 모듈 | 완성도가 높고 기구부 설계 부담이 적다. | 비용이 높고 전용 프로토콜에 의존할 수 있다. | 추후 후보 |

## 제어 방식

Raspberry Pi는 `afterveda-bsp/ptz-kmod` 커널 모듈로 Pan/Tilt 서보에 PWM 신호를 출력한다.

```txt
/dev/afterveda_ptz
→ afterveda_ptz 커널 모듈
→ GPIO18 PWM channel 0: Pan Servo
→ GPIO19 PWM channel 1: Tilt Servo
```

기본 제어 명령은 다음으로 둔다.

```txt
left
right
up
down
stop
center
```

`stop`은 모터 전원을 끄는 의미가 아니라 현재 위치를 유지하는 명령으로 시작한다.
비상 정지는 별도 전원 스위치나 차단 회로로 다룬다.

## 전원 원칙

- 서보 전원은 별도 5V 전원을 사용한다.
- Raspberry Pi GPIO는 제어 신호만 담당한다.
- Raspberry Pi GND와 서보 전원 GND는 공통으로 연결한다.
- 서보 전원의 전류 용량은 서보 2개가 동시에 움직일 때를 기준으로 잡는다.
- 테스트 중에는 전원 스위치를 손이 닿는 위치에 둔다.

## 안전 기준

- 시작 각도는 중앙값으로 둔다.
- Pan/Tilt 이동 범위는 소프트웨어에서 제한한다.
- 기구부가 물리 끝에 닿기 전에 소프트웨어 제한이 먼저 걸려야 한다.
- 첫 테스트는 카메라를 장착하지 않고 브래킷과 서보만 연결한 상태에서 진행한다.
- 전원 인가 직후 서보가 급격히 움직이는지 확인한다.

## 1차 테스트 순서

1. Raspberry Pi에서 `afterveda-ptz` 오버레이를 활성화한다.
2. `afterveda_ptz.ko`를 로드하고 `/dev/afterveda_ptz`가 생성되는지 확인한다.

```bash
ls -l /dev/afterveda_ptz
```

3. Pan 서보만 연결해 중앙, 좌, 우 이동을 확인한다.
4. Tilt 서보만 연결해 중앙, 상, 하 이동을 확인한다.
5. Pan/Tilt 두 축을 함께 연결한다.
6. 좌/우/상/하 명령을 확인한다.
7. `stop` 명령으로 현재 위치 유지가 되는지 확인한다.
8. `center` 명령으로 중앙 복귀가 되는지 확인한다.
9. 소프트웨어 각도 제한이 물리 끝에 닿기 전에 동작하는지 확인한다.
10. 카메라를 장착한 뒤 같은 테스트를 반복한다.

## 다음 단계

- 실제 사용할 Pan/Tilt 브래킷과 서보 토크를 최종 확인한다.
- Raspberry Pi에서 `afterveda-ptz` 오버레이와 `afterveda_ptz.ko` 로드를 확인한다.
- `echo "center" | sudo tee /dev/afterveda_ptz`로 PTZ 장치 명령 경로를 확인한다.
- 카메라 장착 전 좌/우/상/하/정지 테스트를 진행한다.
- 물리 끝에 닿기 전에 멈추도록 Pan/Tilt 각도 제한을 캘리브레이션한다.
