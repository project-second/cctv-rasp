#include "rail_client.h"

#include "http_client.h"
#include "utils.h"

#include <cstdlib>

namespace afterveda_onvif {
namespace {

int json_int(const std::string& json, const std::string& key, int fallback) {
    const std::string marker = "\"" + key + "\":";
    const std::size_t pos = json.find(marker);
    if (pos == std::string::npos) {
        return fallback;
    }
    const std::size_t start = pos + marker.size();
    char* end = nullptr;
    const long value = std::strtol(json.c_str() + start, &end, 10);
    return end == json.c_str() + start ? fallback : static_cast<int>(value);
}

}  // namespace

std::vector<VideoProfile> default_profiles() {
    return {
        {"low", "low", 640, 360, 30, 800},
        {"main", "main", 1280, 720, 30, 2500},
        {"high", "high", 1920, 1080, 30, 5000},
    };
}

std::vector<VideoProfile> profiles_from_rail(const Config& config) {
    std::vector<VideoProfile> profiles = default_profiles();
    const auto current = http_request(config.rail_control_url, "GET", "/profile");
    if (!current.has_value()) {
        return profiles;
    }

    const std::string name = first_nonempty({
        extract_between(*current, "\"profile\": \"", "\""),
        extract_between(*current, "\"profile\":\"", "\""),
    });
    for (auto& profile : profiles) {
        if (profile.token == name) {
            profile.width = json_int(*current, "width", profile.width);
            profile.height = json_int(*current, "height", profile.height);
            profile.fps = json_int(*current, "fps", profile.fps);
            profile.bitrate_kbps = json_int(*current, "bitrate_kbps", profile.bitrate_kbps);
        }
    }
    return profiles;
}

VideoProfile find_profile(const Config& config, const std::string& token) {
    for (const auto& profile : profiles_from_rail(config)) {
        if (profile.token == token || profile.name == token) {
            return profile;
        }
    }
    return default_profiles()[1];
}

void switch_rail_profile(const Config& config, const std::string& token) {
    if (!token.empty()) {
        http_request(config.rail_control_url, "POST", "/profile/" + token);
    }
}

}  // namespace afterveda_onvif
