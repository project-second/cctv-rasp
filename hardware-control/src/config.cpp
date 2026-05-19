#include "config.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace rail_hardware {

namespace {

void print_usage(const char* program) {
    std::cout
        << "Usage: " << program << " [options] <command>\n\n"
        << "Commands:\n"
        << "  left     Move pan left by --step degrees\n"
        << "  right    Move pan right by --step degrees\n"
        << "  up       Move tilt up by --step degrees\n"
        << "  down     Move tilt down by --step degrees\n"
        << "  stop     Re-apply the current position\n"
        << "  center   Move pan and tilt to --center\n\n"
        << "Options:\n"
        << "  --i2c-device <path>     I2C device. Default: /dev/i2c-1\n"
        << "  --address <value>       PCA9685 address. Default: 0x40\n"
        << "  --pan-channel <n>       Pan PWM channel. Default: 0\n"
        << "  --tilt-channel <n>      Tilt PWM channel. Default: 1\n"
        << "  --pan-min <deg>         Pan minimum angle. Default: 30\n"
        << "  --pan-max <deg>         Pan maximum angle. Default: 150\n"
        << "  --tilt-min <deg>        Tilt minimum angle. Default: 45\n"
        << "  --tilt-max <deg>        Tilt maximum angle. Default: 135\n"
        << "  --center <deg>          Center angle. Default: 90\n"
        << "  --step <deg>            Direction step. Default: 5\n"
        << "  --state-file <path>     Position state file. Default: /tmp/rail-hardware-control.state\n"
        << "  --dry-run               Print calculated PWM without touching I2C\n"
        << "  --help                  Show this help\n";
}

int parse_int(const std::string& value, const std::string& name) {
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 0);
    if (*end != '\0') {
        throw std::runtime_error("invalid " + name + ": " + value);
    }
    return static_cast<int>(parsed);
}

void require_range(const std::string& name, int value, int min, int max) {
    if (value < min || value > max) {
        throw std::runtime_error(name + " must be between " + std::to_string(min) + " and " + std::to_string(max));
    }
}

}  // namespace

Config parse_args(int argc, char* argv[]) {
    Config config;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto require_value = [&](const std::string& option) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("missing value for " + option);
            }
            return argv[++i];
        };

        if (arg == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        } else if (arg == "--dry-run") {
            config.dry_run = true;
        } else if (arg == "--i2c-device") {
            config.i2c_device = require_value(arg);
        } else if (arg == "--state-file") {
            config.state_file = require_value(arg);
        } else if (arg == "--address") {
            config.address = parse_int(require_value(arg), arg);
        } else if (arg == "--pan-channel") {
            config.pan_channel = parse_int(require_value(arg), arg);
        } else if (arg == "--tilt-channel") {
            config.tilt_channel = parse_int(require_value(arg), arg);
        } else if (arg == "--pan-min") {
            config.pan_min = parse_int(require_value(arg), arg);
        } else if (arg == "--pan-max") {
            config.pan_max = parse_int(require_value(arg), arg);
        } else if (arg == "--tilt-min") {
            config.tilt_min = parse_int(require_value(arg), arg);
        } else if (arg == "--tilt-max") {
            config.tilt_max = parse_int(require_value(arg), arg);
        } else if (arg == "--center") {
            config.center = parse_int(require_value(arg), arg);
        } else if (arg == "--step") {
            config.step = parse_int(require_value(arg), arg);
        } else if (!arg.empty() && arg.front() == '-') {
            throw std::runtime_error("unknown option: " + arg);
        } else if (config.command.empty()) {
            config.command = arg;
        } else {
            throw std::runtime_error("unexpected argument: " + arg);
        }
    }

    if (config.command.empty()) {
        throw std::runtime_error("missing command");
    }
    if (config.command != "left" && config.command != "right" && config.command != "up" &&
        config.command != "down" && config.command != "stop" && config.command != "center") {
        throw std::runtime_error("command must be left, right, up, down, stop, or center");
    }

    require_range("--address", config.address, 0x03, 0x77);
    require_range("--pan-channel", config.pan_channel, 0, 15);
    require_range("--tilt-channel", config.tilt_channel, 0, 15);
    require_range("--pan-min", config.pan_min, 0, 180);
    require_range("--pan-max", config.pan_max, 0, 180);
    require_range("--tilt-min", config.tilt_min, 0, 180);
    require_range("--tilt-max", config.tilt_max, 0, 180);
    require_range("--center", config.center, 0, 180);
    require_range("--step", config.step, 1, 90);

    if (config.pan_min >= config.pan_max) {
        throw std::runtime_error("--pan-min must be lower than --pan-max");
    }
    if (config.tilt_min >= config.tilt_max) {
        throw std::runtime_error("--tilt-min must be lower than --tilt-max");
    }
    if (config.center < config.pan_min || config.center > config.pan_max ||
        config.center < config.tilt_min || config.center > config.tilt_max) {
        throw std::runtime_error("--center must be inside both pan and tilt ranges");
    }

    return config;
}

}  // namespace rail_hardware
