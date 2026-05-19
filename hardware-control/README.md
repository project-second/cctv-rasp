# Hardware Control

Raspberry Pi에서 PCA9685 I2C PWM 드라이버를 통해 Pan/Tilt 서보를 제어하는 CLI 도구다.

## 하드웨어 구성

- Raspberry Pi 4/5 64-bit
- PCA9685 I2C PWM 드라이버
- Pan 서보: PCA9685 channel 0
- Tilt 서보: PCA9685 channel 1
- 외부 5V 서보 전원
- Raspberry Pi GND와 외부 전원 GND 공통 연결

## 빌드

```bash
cmake -S hardware-control -B hardware-control/build
cmake --build hardware-control/build
```

크로스 컴파일은 미디어 서버와 같은 Raspberry Pi sysroot/toolchain 구성을 사용하되 소스 경로만 `hardware-control`로 바꾼다.

## 실행

하드웨어 연결 전에는 `--dry-run`으로 각도와 PWM count만 확인한다.

```bash
hardware-control/build/hardware-control --dry-run center
hardware-control/build/hardware-control --dry-run left
hardware-control/build/hardware-control --dry-run up
```

Raspberry Pi에서 PCA9685가 보이는지 먼저 확인한다.

```bash
i2cdetect -y 1
```

PCA9685 기본 주소 `0x40`이 보이면 실제 제어를 실행한다.

```bash
hardware-control/build/hardware-control center
hardware-control/build/hardware-control left
hardware-control/build/hardware-control right
hardware-control/build/hardware-control up
hardware-control/build/hardware-control down
hardware-control/build/hardware-control stop
```

## 명령

- `center`: Pan/Tilt를 중앙 각도로 이동한다.
- `left`, `right`: Pan 각도를 `--step`만큼 이동한다.
- `up`, `down`: Tilt 각도를 `--step`만큼 이동한다.
- `stop`: 현재 각도를 다시 적용해 위치를 유지한다. 전원 차단이나 비상 정지가 아니다.

CLI는 매번 종료되므로 마지막 각도는 기본적으로 `/tmp/rail-hardware-control.state`에 저장한다.

## 주요 옵션

```bash
--i2c-device <path>   기본값: /dev/i2c-1
--address <value>     기본값: 0x40
--pan-channel <n>     기본값: 0
--tilt-channel <n>    기본값: 1
--pan-min <deg>       기본값: 30
--pan-max <deg>       기본값: 150
--tilt-min <deg>      기본값: 45
--tilt-max <deg>      기본값: 135
--center <deg>        기본값: 90
--step <deg>          기본값: 5
--state-file <path>   기본값: /tmp/rail-hardware-control.state
--dry-run             I2C 접근 없이 계산값만 출력
```

## 안전 테스트 순서

1. Raspberry Pi에서 I2C를 활성화한다.
2. PCA9685만 연결하고 `i2cdetect -y 1`로 주소를 확인한다.
3. 카메라 없이 Pan 서보만 연결하고 `center`, `left`, `right`를 확인한다.
4. 카메라 없이 Tilt 서보만 연결하고 `center`, `up`, `down`을 확인한다.
5. 두 축을 함께 연결하고 모든 명령을 확인한다.
6. `--pan-min`, `--pan-max`, `--tilt-min`, `--tilt-max`가 물리 끝보다 안쪽에서 멈추도록 조정한다.
7. 카메라를 장착한 뒤 같은 테스트를 반복한다.

서보 전원은 Raspberry Pi 5V 핀에 직접 의존하지 않는다. 테스트 중에는 외부 5V 전원을 손으로 즉시 끌 수 있게 둔다.
