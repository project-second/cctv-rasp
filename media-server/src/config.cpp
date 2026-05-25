#include "config.h"

#include "profiles.h"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace rail_media {

namespace {

void print_usage(const char* program) {
    std::cout
        << "Usage: " << program << " [options]\n\n"
        << "Options:\n"
        << "  --profile <name>     Video profile: low, main, or high. Default: main\n"
        << "  --port <port>        RTSP port. Default: 8554\n"
        << "  --control-port <p>   HTTP control port. Default: 8081\n"
        << "  --mount <path>       RTSP mount path. Default: /live\n"
        << "  --encoder <name>     h264 encoder: v4l2 or x264. Default: v4l2\n"
        << "  --rtsp-user <user>   Enable RTSP Digest auth with this user\n"
        << "  --rtsp-password <p>  RTSP Digest auth password\n"
        << "  --rtsp-realm <name>  RTSP Digest auth realm. Default: rail-media\n"
        << "  --width <pixels>     Frame width. Default: 1280\n"
        << "  --height <pixels>    Frame height. Default: 720\n"
        << "  --fps <fps>          Frame rate. Default: 30\n"
        << "  --bitrate-kbps <k>   Target video bitrate in kbps. Default: profile value\n"
        << "  --help               Show this help\n";
}

}  // namespace

int parse_int(const std::string& value, const std::string& name) {
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (*end != '\0' || parsed <= 0) {
        throw std::runtime_error("invalid " + name + ": " + value);
    }
    return static_cast<int>(parsed);
}

Config parse_args(int argc, char* argv[]) {
    Config config;
    std::string profile_name = "main";
    std::optional<std::string> encoder;
    std::optional<int> width;
    std::optional<int> height;
    std::optional<int> fps;
    std::optional<int> bitrate_kbps;
    std::optional<std::string> rtsp_user;
    std::optional<std::string> rtsp_password;

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
        } else if (arg == "--profile") {
            profile_name = require_value(arg);
        } else if (arg == "--port") {
            config.port = require_value(arg);
        } else if (arg == "--control-port") {
            config.control_port = require_value(arg);
        } else if (arg == "--mount") {
            config.mount = require_value(arg);
        } else if (arg == "--encoder") {
            encoder = require_value(arg);
        } else if (arg == "--rtsp-user") {
            rtsp_user = require_value(arg);
        } else if (arg == "--rtsp-password") {
            rtsp_password = require_value(arg);
        } else if (arg == "--rtsp-realm") {
            config.rtsp_realm = require_value(arg);
        } else if (arg == "--width") {
            width = parse_int(require_value(arg), arg);
        } else if (arg == "--height") {
            height = parse_int(require_value(arg), arg);
        } else if (arg == "--fps") {
            fps = parse_int(require_value(arg), arg);
        } else if (arg == "--bitrate-kbps") {
            bitrate_kbps = parse_int(require_value(arg), arg);
        } else {
            throw std::runtime_error("unknown option: " + arg);
        }
    }

    const Profile* profile = find_profile(profile_name);
    if (profile == nullptr) {
        throw std::runtime_error("--profile must be low, main, or high");
    }
    apply_profile(config, *profile);

    if (encoder.has_value()) {
        config.encoder = *encoder;
    }
    if (width.has_value()) {
        config.width = *width;
    }
    if (height.has_value()) {
        config.height = *height;
    }
    if (fps.has_value()) {
        config.fps = *fps;
    }
    if (bitrate_kbps.has_value()) {
        config.bitrate_kbps = *bitrate_kbps;
    }
    if (rtsp_user.has_value()) {
        config.rtsp_user = *rtsp_user;
    }
    if (rtsp_password.has_value()) {
        config.rtsp_password = *rtsp_password;
    }

    if (config.mount.empty() || config.mount.front() != '/') {
        throw std::runtime_error("--mount must start with /");
    }
    if (config.encoder != "v4l2" && config.encoder != "x264") {
        throw std::runtime_error("--encoder must be v4l2 or x264");
    }
    if (config.rtsp_realm.empty()) {
        throw std::runtime_error("--rtsp-realm must not be empty");
    }
    if (rtsp_user.has_value() != rtsp_password.has_value()) {
        throw std::runtime_error("--rtsp-user and --rtsp-password must be provided together");
    }
    if (rtsp_user.has_value() && (config.rtsp_user.empty() || config.rtsp_password.empty())) {
        throw std::runtime_error("--rtsp-user and --rtsp-password must not be empty");
    }

    return config;
}

}  // namespace rail_media
