#include "config.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace afterveda_onvif {
namespace {

int parse_int(const std::string& value, const std::string& option) {
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (*end != '\0' || parsed <= 0 || parsed > 65535) {
        throw std::runtime_error("invalid " + option + ": " + value);
    }
    return static_cast<int>(parsed);
}

void print_usage(const char* program) {
    std::cout
        << "Usage: " << program << " [options]\n\n"
        << "Options:\n"
        << "  --device-name <name>       ONVIF device name. Default: afterveda-camera\n"
        << "  --manufacturer <name>      Manufacturer. Default: Afterveda\n"
        << "  --model <name>             Model. Default: Rail CCTV\n"
        << "  --firmware <version>       Firmware version. Default: 0.1.0\n"
        << "  --serial <value>           Serial number. Default: afterveda-001\n"
        << "  --hardware-id <value>      Hardware ID. Default: afterveda-picam\n"
        << "  --xaddr-host <host>        Host advertised in ONVIF XAddrs. Default: 127.0.0.1\n"
        << "  --onvif-port <port>        ONVIF HTTP port. Default: 8000\n"
        << "  --rtsp-uri <uri>           RTSP URI returned by GetStreamUri\n"
        << "  --rail-control-url <url>   rail-media HTTP control URL. Default: http://127.0.0.1:8081\n"
        << "  --hardware-control <path>  hardware-control binary path\n"
        << "  --ptz-step <deg>           Hardware control step for PTZ. Default: 5\n"
        << "  --ptz-dry-run              Add --dry-run to hardware-control calls\n"
        << "  --username <user>          Require ONVIF UsernameToken username\n"
        << "  --password <password>      Require password text in UsernameToken\n"
        << "  --help                     Show this help\n";
}

}  // namespace

Config parse_args(int argc, char* argv[]) {
    Config config;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto require_value = [&](const std::string& option) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("missing value for " + option);
            }
            return argv[++i];
        };

        if (arg == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        } else if (arg == "--device-name") {
            config.device_name = require_value(arg);
        } else if (arg == "--manufacturer") {
            config.manufacturer = require_value(arg);
        } else if (arg == "--model") {
            config.model = require_value(arg);
        } else if (arg == "--firmware") {
            config.firmware = require_value(arg);
        } else if (arg == "--serial") {
            config.serial = require_value(arg);
        } else if (arg == "--hardware-id") {
            config.hardware_id = require_value(arg);
        } else if (arg == "--xaddr-host") {
            config.xaddr_host = require_value(arg);
        } else if (arg == "--onvif-port") {
            config.onvif_port = parse_int(require_value(arg), arg);
        } else if (arg == "--rtsp-uri") {
            config.rtsp_uri = require_value(arg);
        } else if (arg == "--rail-control-url") {
            config.rail_control_url = require_value(arg);
        } else if (arg == "--hardware-control") {
            config.hardware_control = require_value(arg);
        } else if (arg == "--ptz-step") {
            config.ptz_step = parse_int(require_value(arg), arg);
        } else if (arg == "--ptz-dry-run") {
            config.ptz_dry_run = true;
        } else if (arg == "--username") {
            config.username = require_value(arg);
        } else if (arg == "--password") {
            config.password = require_value(arg);
        } else {
            throw std::runtime_error("unknown option: " + arg);
        }
    }

    if (config.xaddr_host.empty() || config.rtsp_uri.empty()) {
        throw std::runtime_error("--xaddr-host and --rtsp-uri must not be empty");
    }
    if (config.username.empty() != config.password.empty()) {
        throw std::runtime_error("--username and --password must be provided together");
    }
    return config;
}

}  // namespace afterveda_onvif
