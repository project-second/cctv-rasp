#pragma once

#include <string>

namespace afterveda_onvif {

struct VideoProfile {
    std::string token;
    std::string name;
    int width = 1280;
    int height = 720;
    int fps = 30;
    int bitrate_kbps = 2500;
};

struct HttpResponse {
    int status = 200;
    std::string reason = "OK";
    std::string content_type = "application/soap+xml; charset=utf-8";
    std::string body;
};

}  // namespace afterveda_onvif
