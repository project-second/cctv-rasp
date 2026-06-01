# Afterveda ONVIF Server

`afterveda-onvif` exposes the existing `rail-media` RTSP stream as an ONVIF-like IP camera.

The process does not encode video. It answers ONVIF discovery and SOAP requests, then returns the RTSP stream URI served by `rail-media`.

This module is moving toward ONVIF Profile T device behavior, but it is not a certified Profile T conformant implementation yet. Treat the current implementation as Profile T work in progress until it passes the official ONVIF Device Test Tool.

## Build

```bash
cmake -S onvif-server -B onvif-server/build
cmake --build onvif-server/build
```

## Run

```bash
onvif-server/build/afterveda-onvif \
  --device-name afterveda-camera \
  --xaddr-host 192.168.0.32 \
  --rtsp-uri rtsp://192.168.0.32:8554/live
```

Useful options:

```txt
--onvif-port <port>          HTTP SOAP port. Default: 8000
--xaddr-host <ip-or-host>    Host advertised in ONVIF service URLs
--rtsp-uri <uri>             Stream URI returned by GetStreamUri
--rail-control-url <url>     rail-media control base URL. Default: http://127.0.0.1:8081
--hardware-control <path>    hardware-control binary for PTZ commands
--username <user>            Require ONVIF UsernameToken username
--password <password>        Require ONVIF UsernameToken password text to be present
```

## Supported ONVIF surface

- WS-Discovery Probe and Resolve responses
- Device: `GetCapabilities`, `GetDeviceInformation`, `GetServices`, `GetSystemDateAndTime`, `GetScopes`, `GetHostname`, `GetNetworkInterfaces`, `GetUsers`, `GetServiceCapabilities`
- Media: `GetProfiles`, `GetStreamUri`, `GetVideoEncoderConfiguration`, `SetVideoEncoderConfiguration`
- Media2: basic H.264 `GetProfiles`, `GetStreamUri`, `GetServiceCapabilities`
- PTZ: `GetNodes`, `GetConfigurations`, `GetPresets`, `SetPreset`, `GotoPreset`, `ContinuousMove`, `RelativeMove`, `Stop`, `GetStatus`, `GotoHomePosition`, `SetHomePosition`
- Imaging: basic `GetOptions`, `GetImagingSettings`, `SetImagingSettings` compatibility responses
- Events: basic `GetEventProperties`, `CreatePullPointSubscription`, `PullMessages`, `Renew`, `Unsubscribe` compatibility responses
- OSD: basic text OSD query/update compatibility responses
- WS-Security: UsernameToken `PasswordText` and `PasswordDigest` checks when `--username` and `--password` are provided

PTZ maps ONVIF pan/tilt commands to the existing `hardware-control` CLI. Zoom requests return a SOAP fault because no zoom hardware is currently configured.
