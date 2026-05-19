#include "profiles.h"

#include <array>

namespace rail_media {

namespace {

constexpr std::array<Profile, 3> kProfiles = {{
    {"low", "360p", 640, 360, 30, 800, "v4l2"},
    {"main", "720p", 1280, 720, 30, 2500, "v4l2"},
    {"high", "1080p", 1920, 1080, 30, 5000, "v4l2"},
}};

}  // namespace

const Profile* find_profile(const std::string& name) {
    for (const auto& profile : kProfiles) {
        if (name == profile.name) {
            return &profile;
        }
    }
    return nullptr;
}

const Profile* profiles_begin() {
    return kProfiles.data();
}

const Profile* profiles_end() {
    return kProfiles.data() + kProfiles.size();
}

std::size_t profile_count() {
    return kProfiles.size();
}

void apply_profile(Config& config, const Profile& profile) {
    config.profile = profile.name;
    config.encoder = profile.encoder;
    config.width = profile.width;
    config.height = profile.height;
    config.fps = profile.fps;
    config.bitrate_kbps = profile.bitrate_kbps;
}

}  // namespace rail_media
