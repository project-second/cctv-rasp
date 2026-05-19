#pragma once

#include "config.h"
#include "pan_tilt.h"

namespace rail_hardware {

int angle_to_count(const Config& config, int angle);
void write_servos(const Config& config, const Position& position);

}  // namespace rail_hardware
