#pragma once

#include "config.h"

#include <string>

namespace afterveda_onvif {

struct PtzPosition {
    int pan;
    int tilt;
};

struct PtzStatus {
    PtzPosition position;
    int pan_speed;
    int tilt_speed;
};

PtzPosition home_ptz_position();
PtzPosition clamp_ptz_position(PtzPosition position);
double ptz_pan_to_onvif(int pan);
double ptz_tilt_to_onvif(int tilt);

std::string read_ptz_position(const Config& config, PtzPosition& out);
std::string read_ptz_status(const Config& config, PtzStatus& out);
std::string write_ptz_position(const Config& config, const PtzPosition& position);
std::string continuous_move_ptz(const Config& config, double x, double y, int timeout_ms);
std::string relative_move_ptz(const Config& config, double x, double y);
std::string stop_ptz(const Config& config);
std::string home_ptz(const Config& config);
std::string set_home_ptz_from_current(const Config& config);

}  // namespace afterveda_onvif
