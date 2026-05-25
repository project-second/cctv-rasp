#pragma once

#include <string>

namespace afterveda_onvif {

struct Config {
    std::string device_name = "afterveda-camera";
    std::string manufacturer = "Afterveda";
    std::string model = "Rail CCTV";
    std::string firmware = "0.1.0";
    std::string serial = "afterveda-001";
    std::string hardware_id = "afterveda-picam";
    std::string xaddr_host = "127.0.0.1";
    std::string rtsp_uri = "rtsp://127.0.0.1:8554/live";
    std::string rail_control_url = "http://127.0.0.1:8081";
    std::string hardware_control = "hardware-control/build/hardware-control";
    std::string username;
    std::string password;
    int onvif_port = 8000;
    int ptz_step = 5;
    bool ptz_dry_run = false;
};

Config parse_args(int argc, char* argv[]);

}  // namespace afterveda_onvif
