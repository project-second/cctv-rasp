#include "state_store.h"

#include <fstream>
#include <stdexcept>

namespace rail_hardware {

Position load_position(const Config& config) {
    Position position = centered_position(config);
    std::ifstream input(config.state_file);
    if (!input) {
        return position;
    }

    int pan = 0;
    int tilt = 0;
    if (input >> pan >> tilt) {
        position = clamp_position(config, Position{pan, tilt});
    }
    return position;
}

void save_position(const Config& config, const Position& position) {
    std::ofstream output(config.state_file, std::ios::trunc);
    if (!output) {
        throw std::runtime_error("failed to write state file: " + config.state_file);
    }
    output << position.pan << " " << position.tilt << "\n";
}

}  // namespace rail_hardware
