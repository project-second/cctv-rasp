# ONVIF WS-Discovery 자동 검색 문제 해결

## 문제

ONVIF Device Manager(ODM)에서 수동 등록은 가능했지만 자동 검색이 불안정했다. 최초 실행 직후에는 장치가 보이다가 Refresh 후 목록에서 사라지거나, Probe 요청은 들어오는데 장치가 자동 등록되지 않았다.

## 증상

Raspberry Pi에서 `tcpdump`로 확인하면 ODM의 Probe 요청과 서버의 응답은 모두 보였다.

```bash
sudo tcpdump -ni wlan0 udp port 3702
```

초기 응답은 다음처럼 UDP payload가 2400바이트 이상이었다.

```text
192.168.0.21.xxxxx > 239.255.255.250.3702: UDP, length 749
192.168.0.32.3702 > 192.168.0.21.xxxxx: UDP, length 2467
```

응답은 나가고 있었지만 ODM은 Refresh 시 장치를 안정적으로 채택하지 못했다.

## 원인

WS-Discovery는 UDP 3702를 사용한다. 기존 gSOAP 자동 `ProbeMatches` 응답은 전체 ONVIF namespace를 모두 포함해 XML이 커졌고, 응답 UDP payload가 MTU를 초과했다.

결과적으로 IP fragmentation이 발생했다.

- 첫 fragment에는 SOAP Header만 포함
- 중요한 `ProbeMatches`, `XAddrs`, `Types`, `Scopes`는 뒤 fragment로 밀림
- 일부 클라이언트나 방화벽 환경에서 UDP fragment가 누락되면 자동 검색 실패

처음에는 보이지만 Refresh 후 사라지는 현상은, ODM이 새 Probe 결과로 목록을 갱신하는 과정에서 fragment된 응답을 안정적으로 처리하지 못해서 발생한 것으로 판단했다.

## 해결

gSOAP의 기본 WS-Discovery 자동 응답을 우회했다.

- `wsdd_event_Probe`에서 `SOAP_WSDD_ADHOC` 모드 사용
- 자동 `ProbeMatches` 전송 대신 `sendto()`로 최소 XML 응답 직접 전송
- Discovery 응답에 필요한 namespace만 포함
- `Types`, `Scopes`, `XAddrs`, `MetadataVersion`만 명확히 제공

응답 크기를 MTU 아래로 줄여 UDP fragmentation을 제거했다.

## 결과

수정 후 Probe 응답 크기가 2400바이트대에서 1282바이트로 감소했다.

```text
192.168.0.21.xxxxx > 239.255.255.250.3702: UDP, length 749
192.168.0.32.3702 > 192.168.0.21.xxxxx: UDP, length 1282
```

이후 ODM Refresh에서도 서버가 정상적으로 `ProbeMatches`를 반환했고, 자동 검색이 안정화되었다.

## 배운 점

ONVIF Discovery는 단순히 응답을 보내는 것만으로 충분하지 않다. UDP 기반 프로토콜에서는 응답 크기와 fragmentation 여부가 클라이언트 호환성에 직접 영향을 준다.

이번 문제는 SOAP XML 내용 자체보다도, gSOAP이 생성한 과도한 namespace로 인해 네트워크 계층에서 응답이 분할된 것이 핵심 원인이었다.

## 검증 명령

```bash
sudo tcpdump -ni wlan0 udp port 3702
```

응답 UDP length가 MTU 이하인지 확인한다.

```bash
sudo tcpdump -ni wlan0 -s0 -A 'udp src port 3702' -c 1
```

응답 XML에 다음 값이 포함되는지 확인한다.

```text
ProbeMatches
dn:NetworkVideoTransmitter
tds:Device
http://<device-ip>:8000/onvif/device_service
```
