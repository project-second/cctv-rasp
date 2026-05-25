#pragma once

#include <string>

namespace rail_media {

struct Config {
    std::string profile = "main";
    std::string port = "8554";
    std::string control_port = "8081";
    std::string mount = "/live";
    std::string encoder = "v4l2";
    std::string rtsp_user;
    std::string rtsp_password;
    std::string rtsp_realm = "rail-media";
    int width = 1280;
    int height = 720;
    int fps = 30;
    int bitrate_kbps = 2500;
};

int parse_int(const std::string& value, const std::string& name);
Config parse_args(int argc, char* argv[]);

}  // namespace rail_media
