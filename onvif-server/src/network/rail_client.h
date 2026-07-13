#pragma once

#include "config.h"
#include "types.h"

#include <string>
#include <vector>

namespace afterveda_onvif {

std::vector<VideoProfile> default_profiles();
std::vector<VideoProfile> profiles_from_rail(const Config& config);
VideoProfile find_profile(const Config& config, const std::string& token);
std::string set_video_config(const Config& config, const VideoProfile& profile);
ImageSettings imaging_settings_from_rail(const Config& config);
std::string set_imaging_settings(const Config& config, const ImageSettings& settings);
OsdSettings osd_settings_from_rail(const Config& config);
std::string set_osd_settings(const Config& config, const OsdSettings& settings);

}  // namespace afterveda_onvif
