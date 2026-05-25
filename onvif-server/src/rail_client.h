#pragma once

#include "config.h"
#include "types.h"

#include <string>
#include <vector>

namespace afterveda_onvif {

std::vector<VideoProfile> default_profiles();
std::vector<VideoProfile> profiles_from_rail(const Config& config);
VideoProfile find_profile(const Config& config, const std::string& token);
void switch_rail_profile(const Config& config, const std::string& token);

}  // namespace afterveda_onvif
