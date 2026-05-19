#include "pan_tilt.h"

#include <algorithm>

namespace rail_hardware {

Position centered_position(const Config& config) {
    return Position{config.center, config.center};
}

Position clamp_position(const Config& config, Position position) {
    position.pan = std::clamp(position.pan, config.pan_min, config.pan_max);
    position.tilt = std::clamp(position.tilt, config.tilt_min, config.tilt_max);
    return position;
}

Position apply_command(const Config& config, Position position) {
    if (config.command == "left") {
        position.pan -= config.step;
    } else if (config.command == "right") {
        position.pan += config.step;
    } else if (config.command == "up") {
        position.tilt -= config.step;
    } else if (config.command == "down") {
        position.tilt += config.step;
    } else if (config.command == "center") {
        position = centered_position(config);
    }
    return clamp_position(config, position);
}

}  // namespace rail_hardware
