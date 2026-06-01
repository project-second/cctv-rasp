# ONVIF Server

Afterveda uses a separate `afterveda-onvif` process to expose the existing RTSP stream as an ONVIF camera.

The ONVIF server is a Profile T implementation in progress. It advertises and implements several Profile T-oriented services, but it must not be described as Profile T conformant until it passes the official ONVIF Device Test Tool.

## Runtime shape

```txt
rail-media
  - PiCam capture
  - GStreamer RTSP stream
  - HTTP profile control

afterveda-onvif
  - WS-Discovery UDP response
- ONVIF Device/Media/PTZ SOAP endpoints
- ONVIF Media2/OSD/Profile T-oriented compatibility endpoints
  - GetStreamUri returns the rail-media RTSP URI
  - PTZ requests call hardware-control
```

## Build

```bash
cmake -S onvif-server -B onvif-server/build
cmake --build onvif-server/build
```

## Run

```bash
media-server/build/rail-media --profile main

onvif-server/build/afterveda-onvif \
  --device-name afterveda-camera \
  --xaddr-host <pi-ip> \
  --rtsp-uri rtsp://<pi-ip>:8554/live \
  --rail-control-url http://127.0.0.1:8081 \
  --hardware-control hardware-control/build/hardware-control
```

For hardware-free PTZ testing, add `--ptz-dry-run`.

## Implemented scope

- WS-Discovery `Probe` and `Resolve` responses
- Device service: `GetCapabilities`, `GetDeviceInformation`, `GetServices`, `GetSystemDateAndTime`, `GetScopes`, `GetHostname`, `GetNetworkInterfaces`, `GetUsers`, `GetServiceCapabilities`
- Media service: `GetProfiles`, `GetStreamUri`, `GetVideoEncoderConfiguration`, `SetVideoEncoderConfiguration`
- Media2 service: basic H.264 `GetProfiles`, `GetStreamUri`, `GetServiceCapabilities`
- PTZ service: `GetNodes`, `GetConfigurations`, `GetPresets`, `SetPreset`, `GotoPreset`, `ContinuousMove`, `RelativeMove`, `Stop`, `GetStatus`, `GotoHomePosition`, `SetHomePosition`
- Imaging service: basic option/settings compatibility responses
- Events service: basic event properties and PullPoint compatibility responses
- OSD service: basic text OSD compatibility responses
- Optional UsernameToken `PasswordText` and `PasswordDigest` gate using `--username` and `--password`

Imaging settings are compatibility responses for now; they do not change real sensor controls until camera control plumbing is added.
Metadata streaming and real OSD overlay are not complete yet; those require `rail-media` pipeline work.
