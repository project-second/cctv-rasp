#include "media_model.h"

#include "rail_client.h"

namespace afterveda_onvif {

MediaModel media_model_from_config(const Config& config) {
    return {profiles_from_rail(config), config.rtsp_uri};
}

std::string encoder_token_for_profile(const VideoProfile& profile) {
    return "encoder-" + profile.token;
}

std::string profile_token_from_encoder_token(const std::string& token) {
    static constexpr const char* kPrefix = "encoder-";
    const std::string prefix = kPrefix;
    if (token.rfind(prefix, 0) == 0) {
        return token.substr(prefix.size());
    }
    return token;
}

bool has_profile_token(const MediaModel& model, const std::string& token) {
    const std::string normalized = profile_token_from_encoder_token(token);
    for (const auto& profile : model.profiles) {
        if (profile.token == normalized || profile.name == normalized) {
            return true;
        }
    }
    return false;
}

VideoProfile profile_for_token(const MediaModel& model, const std::string& token) {
    const std::string normalized = profile_token_from_encoder_token(token);
    for (const auto& profile : model.profiles) {
        if (profile.token == normalized || profile.name == normalized) {
            return profile;
        }
    }
    for (const auto& profile : model.profiles) {
        if (profile.token == "main") {
            return profile;
        }
    }
    return model.profiles.empty() ? VideoProfile{} : model.profiles.front();
}

}  // namespace afterveda_onvif
