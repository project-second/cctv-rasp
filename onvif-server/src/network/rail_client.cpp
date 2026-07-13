#include "rail_client.h"

#include "http_client.h"
#include "utils.h"

#include <cstdlib>
#include <sstream>

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

double json_double(const std::string& json, const std::string& key, double fallback) {
    const std::string marker = "\"" + key + "\":";
    const std::size_t pos = json.find(marker);
    if (pos == std::string::npos) {
        return fallback;
    }
    const std::size_t start = pos + marker.size();
    char* end = nullptr;
    const double value = std::strtod(json.c_str() + start, &end);
    return end == json.c_str() + start ? fallback : value;
}

bool json_bool(const std::string& json, const std::string& key, bool fallback) {
    const std::string marker = "\"" + key + "\":";
    const std::size_t pos = json.find(marker);
    if (pos == std::string::npos) {
        return fallback;
    }
    const std::size_t start = json.find_first_not_of(" \t\r\n", pos + marker.size());
    if (start == std::string::npos) {
        return fallback;
    }
    if (json.compare(start, 4, "true") == 0) {
        return true;
    }
    if (json.compare(start, 5, "false") == 0) {
        return false;
    }
    return fallback;
}

std::string json_string(const std::string& json, const std::string& key, const std::string& fallback) {
    const std::string marker = "\"" + key + "\":";
    const std::size_t pos = json.find(marker);
    if (pos == std::string::npos) {
        return fallback;
    }
    std::size_t start = json.find('"', pos + marker.size());
    if (start == std::string::npos) {
        return fallback;
    }
    ++start;

    std::string value;
    for (std::size_t i = start; i < json.size(); ++i) {
        const char c = json[i];
        if (c == '"') {
            return value;
        }
        if (c == '\\' && i + 1 < json.size()) {
            value += json[++i];
        } else {
            value += c;
        }
    }
    return fallback;
}

std::string json_escape(const std::string& value) {
    std::string escaped;
    for (const char c : value) {
        if (c == '"' || c == '\\') {
            escaped += '\\';
        }
        escaped += c;
    }
    return escaped;
}

}  // namespace

std::vector<VideoProfile> default_profiles() {
    return {
        {"main", "main", 1280, 720, 30, 2500},
    };
}

std::vector<VideoProfile> profiles_from_rail(const Config& config) {
    std::vector<VideoProfile> profiles = default_profiles();
    const auto current = http_request(config.rail_control_url, "GET", "/profile");
    if (!current.has_value()) {
        return profiles;
    }

    VideoProfile& profile = profiles.front();
    profile.width = json_int(*current, "width", profile.width);
    profile.height = json_int(*current, "height", profile.height);
    profile.fps = json_int(*current, "fps", profile.fps);
    profile.bitrate_kbps = json_int(*current, "bitrate_kbps", profile.bitrate_kbps);
    return profiles;
}

VideoProfile find_profile(const Config& config, const std::string& token) {
    for (const auto& profile : profiles_from_rail(config)) {
        if (profile.token == token || profile.name == token) {
            return profile;
        }
    }
    return default_profiles()[0];
}

std::string set_video_config(const Config& config, const VideoProfile& profile) {
    std::ostringstream body;
    body << "{\"width\":" << profile.width
         << ",\"height\":" << profile.height
         << ",\"fps\":" << profile.fps
         << ",\"bitrate_kbps\":" << profile.bitrate_kbps
         << "}";
    const auto response = http_request_with_status(config.rail_control_url, "POST", "/config", body.str());
    if (!response.has_value()) {
        return "rail-media video config control is unavailable";
    }
    if (response->status < 200 || response->status >= 300) {
        return response->body.empty() ? "rail-media rejected video config" : response->body;
    }
    return {};
}

ImageSettings imaging_settings_from_rail(const Config& config) {
    ImageSettings settings;
    const auto current = http_request(config.rail_control_url, "GET", "/imaging");
    if (!current.has_value()) {
        return settings;
    }

    settings.brightness = json_double(*current, "brightness", settings.brightness);
    settings.contrast = json_double(*current, "contrast", settings.contrast);
    settings.color_saturation = json_double(*current, "color_saturation", settings.color_saturation);
    return settings;
}

std::string set_imaging_settings(const Config& config, const ImageSettings& settings) {
    std::ostringstream body;
    body << "{\"brightness\":" << settings.brightness
         << ",\"contrast\":" << settings.contrast
         << ",\"color_saturation\":" << settings.color_saturation
         << "}";
    const auto response = http_request_with_status(config.rail_control_url, "POST", "/imaging", body.str());
    if (!response.has_value()) {
        return "rail-media imaging control is unavailable";
    }
    if (response->status < 200 || response->status >= 300) {
        return response->body.empty() ? "rail-media rejected imaging settings" : response->body;
    }
    return {};
}

OsdSettings osd_settings_from_rail(const Config& config) {
    OsdSettings settings;
    const auto current = http_request(config.rail_control_url, "GET", "/osd");
    if (!current.has_value()) {
        return settings;
    }

    settings.enabled = json_bool(*current, "enabled", settings.enabled);
    settings.text = json_string(*current, "text", settings.text);
    return settings;
}

std::string set_osd_settings(const Config& config, const OsdSettings& settings) {
    std::ostringstream body;
    body << "{\"enabled\":" << (settings.enabled ? "true" : "false")
         << ",\"text\":\"" << json_escape(settings.text) << "\"}";
    const auto response = http_request_with_status(config.rail_control_url, "POST", "/osd", body.str());
    if (!response.has_value()) {
        return "rail-media osd control is unavailable";
    }
    if (response->status < 200 || response->status >= 300) {
        return response->body.empty() ? "rail-media rejected osd settings" : response->body;
    }
    return {};
}

}  // namespace afterveda_onvif
