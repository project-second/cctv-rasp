#pragma once

#include "config.h"

#include <string>

namespace afterveda_onvif {

struct PtzPosition {
    int pan;
    int tilt;
};

PtzPosition home_ptz_position();
PtzPosition clamp_ptz_position(PtzPosition position);
double ptz_pan_to_onvif(int pan);
double ptz_tilt_to_onvif(int tilt);

std::string read_ptz_position(const Config& config, PtzPosition& out);
std::string write_ptz_position(const Config& config, const PtzPosition& position);
std::string move_ptz_from_request(const Config& config, const std::string& request);
std::string stop_ptz(const Config& config);
std::string home_ptz(const Config& config);
std::string set_home_ptz_from_current(const Config& config);

}  // namespace afterveda_onvif
