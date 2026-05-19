#pragma once

#include "config.h"
#include "pan_tilt.h"

namespace rail_hardware {

Position load_position(const Config& config);
void save_position(const Config& config, const Position& position);

}  // namespace rail_hardware
