#pragma once

#include "config.h"

#include <cstddef>
#include <string>

namespace rail_media {

struct Profile {
    const char* name;
    const char* quality;
    int width;
    int height;
    int fps;
    int bitrate_kbps;
    const char* encoder;
};

const Profile* find_profile(const std::string& name);
const Profile* profiles_begin();
const Profile* profiles_end();
std::size_t profile_count();
void apply_profile(Config& config, const Profile& profile);

}  // namespace rail_media
