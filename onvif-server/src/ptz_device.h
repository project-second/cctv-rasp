#pragma once

#include "config.h"

#include <string>

namespace afterveda_onvif {

std::string write_ptz_command(const Config& config, const std::string& command);

}  // namespace afterveda_onvif
