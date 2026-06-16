#pragma once

#include "config.h"

#include <string>

namespace afterveda_onvif {

std::string handle_ptz(const Config& config, const std::string& request);

}  // namespace afterveda_onvif
