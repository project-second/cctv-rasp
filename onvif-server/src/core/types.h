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

struct ImageSettings {
    double brightness = 50.0;
    double contrast = 50.0;
    double color_saturation = 50.0;
};

struct OsdSettings {
    bool enabled = true;
    std::string text = "Afterveda";
};

}  // namespace afterveda_onvif
