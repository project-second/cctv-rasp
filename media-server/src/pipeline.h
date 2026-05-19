#pragma once

#include "config.h"

#include <string>

namespace rail_media {

std::string encoder_pipeline(const Config& config);
std::string build_launch_pipeline(const Config& config);

}  // namespace rail_media
