# Afterveda ONVIF Server

`afterveda-onvif` exposes the existing `rail-media` RTSP stream as an ONVIF-like IP camera.

The process does not encode video. It answers ONVIF discovery and SOAP requests, then returns the RTSP stream URI served by `rail-media`.

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
- Device: `GetCapabilities`, `GetDeviceInformation`, `GetServices`, `GetSystemDateAndTime`
- Media: `GetProfiles`, `GetStreamUri`, `GetVideoEncoderConfiguration`, `SetVideoEncoderConfiguration`
- PTZ: `ContinuousMove`, `RelativeMove`, `Stop`, `GetStatus`, `GotoHomePosition`, `SetHomePosition`
- Imaging: basic `GetOptions`, `GetImagingSettings`, `SetImagingSettings` compatibility responses
- Events: basic `GetEventProperties` compatibility response

PTZ maps ONVIF pan/tilt commands to the existing `hardware-control` CLI. Zoom requests return a SOAP fault because no zoom hardware is currently configured.
