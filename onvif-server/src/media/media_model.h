#pragma once

#include "config.h"
#include "types.h"

#include <string>
#include <vector>

namespace afterveda_onvif {

struct MediaModel {
    std::vector<VideoProfile> profiles;
    std::string rtsp_uri;
};

constexpr const char* kVideoSourceToken = "picam-source";
constexpr const char* kVideoSourceConfigToken = "video-source";
constexpr const char* kPtzConfigToken = "ptz";
constexpr const char* kMediaSessionTimeout = "PT60S";

MediaModel media_model_from_config(const Config& config);
std::string encoder_token_for_profile(const VideoProfile& profile);
std::string profile_token_from_encoder_token(const std::string& token);
bool has_profile_token(const MediaModel& model, const std::string& token);
VideoProfile profile_for_token(const MediaModel& model, const std::string& token);

}  // namespace afterveda_onvif
