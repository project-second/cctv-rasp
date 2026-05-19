#pragma once

#include "config.h"

namespace rail_hardware {

struct Position {
    int pan = 90;
    int tilt = 90;
};

Position centered_position(const Config& config);
Position clamp_position(const Config& config, Position position);
Position apply_command(const Config& config, Position position);

}  // namespace rail_hardware
