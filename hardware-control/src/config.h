#pragma once

#include <string>

namespace rail_hardware {

struct Config {
    std::string command;
    std::string i2c_device = "/dev/i2c-1";
    std::string state_file = "/tmp/rail-hardware-control.state";
    int address = 0x40;
    int frequency_hz = 50;
    int pan_channel = 0;
    int tilt_channel = 1;
    int pan_min = 30;
    int pan_max = 150;
    int tilt_min = 45;
    int tilt_max = 135;
    int center = 90;
    int step = 5;
    int pulse_min_us = 500;
    int pulse_max_us = 2500;
    bool dry_run = false;
};

Config parse_args(int argc, char* argv[]);

}  // namespace rail_hardware
