#include "ptz_device.h"

#include <cerrno>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>

namespace afterveda_onvif {
namespace {

constexpr int kPanMin = 30;
constexpr int kPanMax = 150;
constexpr int kTiltMin = 0;
constexpr int kTiltMax = 160;
constexpr int kHomePan = 90;
constexpr int kHomeTilt = 45;

std::mutex g_home_mutex;
std::mutex g_command_mutex;
std::atomic<unsigned long long> g_motion_generation{0};
std::atomic<int> g_dry_run_pan{kHomePan};
std::atomic<int> g_dry_run_tilt{kHomeTilt};
std::atomic<int> g_dry_run_pan_speed{0};
std::atomic<int> g_dry_run_tilt_speed{0};
PtzPosition g_home_position{kHomePan, kHomeTilt};

int clamp_int(int value, int min_value, int max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

std::string position_wire(PtzPosition position) {
    position = clamp_ptz_position(position);
    return "pan=" + std::to_string(position.pan) + " tilt=" + std::to_string(position.tilt) + "\n";
}

std::string speed_wire(int pan_speed, int tilt_speed) {
    return "pan_speed=" + std::to_string(pan_speed) + " tilt_speed=" + std::to_string(tilt_speed) + "\n";
}

std::string write_ptz_command_unlocked(const Config& config, const std::string& wire) {
    if (config.ptz_dry_run) {
        std::cout << "[dry-run] ptz_device=" << config.ptz_device
                  << " command=" << wire;
        return {};
    }

    const int fd = open(config.ptz_device.c_str(), O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        return "failed to open PTZ device " + config.ptz_device + ": " + std::strerror(errno);
    }

    const char* data = wire.data();
    std::size_t remaining = wire.size();
    while (remaining > 0) {
        const ssize_t written = write(fd, data, remaining);
        if (written < 0) {
            const std::string error = "failed to write PTZ command to " + config.ptz_device + ": " + std::strerror(errno);
            close(fd);
            return error;
        }
        data += written;
        remaining -= static_cast<std::size_t>(written);
    }

    if (close(fd) != 0) {
        return "failed to close PTZ device " + config.ptz_device + ": " + std::strerror(errno);
    }
    return {};
}

bool parse_int_after(const std::string& text, const std::string& marker, int& out) {
    const std::size_t marker_pos = text.find(marker);
    if (marker_pos == std::string::npos) {
        return false;
    }

    const char* begin = text.c_str() + marker_pos + marker.size();
    char* end = nullptr;
    const long value = std::strtol(begin, &end, 10);
    if (begin == end) {
        return false;
    }

    out = static_cast<int>(value);
    return true;
}

bool parse_status_text(const std::string& text, PtzStatus& out) {
    int pan = 0;
    int tilt = 0;
    int pan_speed = 0;
    int tilt_speed = 0;
    if (!parse_int_after(text, "pan=", pan) ||
        !parse_int_after(text, "tilt=", tilt) ||
        !parse_int_after(text, "pan_speed=", pan_speed) ||
        !parse_int_after(text, "tilt_speed=", tilt_speed)) {
        return false;
    }
    out.position = clamp_ptz_position({pan, tilt});
    out.pan_speed = pan_speed;
    out.tilt_speed = tilt_speed;
    return true;
}

int delta_from_axis(const Config& config, double value) {
    if (value == 0.0) {
        return 0;
    }

    int delta = static_cast<int>(std::lround(static_cast<double>(config.ptz_step) * std::fabs(value)));
    if (delta == 0) {
        delta = 1;
    }
    return value < 0.0 ? -delta : delta;
}

int speed_from_axis(const Config& config, double value) {
    if (value == 0.0) {
        return 0;
    }

    int speed = static_cast<int>(std::lround(static_cast<double>(config.ptz_speed) * std::fabs(value)));
    if (speed == 0) {
        speed = 1;
    }
    return value < 0.0 ? -speed : speed;
}

double angle_to_onvif(int value, int min_value, int max_value) {
    const double midpoint = (static_cast<double>(min_value) + static_cast<double>(max_value)) / 2.0;
    const double half_range = (static_cast<double>(max_value) - static_cast<double>(min_value)) / 2.0;
    if (half_range == 0.0) {
        return 0.0;
    }
    const double normalized = (static_cast<double>(value) - midpoint) / half_range;
    if (normalized < -1.0) {
        return -1.0;
    }
    if (normalized > 1.0) {
        return 1.0;
    }
    return normalized;
}

std::string move_ptz_by_axes(const Config& config, double x, double y, const std::string& mode) {
    PtzPosition position{};
    const std::string error = read_ptz_position(config, position);
    if (!error.empty()) {
        return error;
    }

    const int pan_delta = delta_from_axis(config, x);
    const int tilt_delta = delta_from_axis(config, y);
    position.pan += pan_delta;
    position.tilt += tilt_delta;

    std::cout << "[info] ptz " << mode
              << " x=" << x
              << " y=" << y
              << " pan_delta=" << pan_delta
              << " tilt_delta=" << tilt_delta
              << " target_pan=" << clamp_ptz_position(position).pan
              << " target_tilt=" << clamp_ptz_position(position).tilt
              << "\n";

    return write_ptz_position(config, clamp_ptz_position(position));
}

}  // namespace

PtzPosition home_ptz_position() {
    std::lock_guard<std::mutex> lock(g_home_mutex);
    return g_home_position;
}

PtzPosition clamp_ptz_position(PtzPosition position) {
    position.pan = clamp_int(position.pan, kPanMin, kPanMax);
    position.tilt = clamp_int(position.tilt, kTiltMin, kTiltMax);
    return position;
}

double ptz_pan_to_onvif(int pan) {
    return angle_to_onvif(clamp_int(pan, kPanMin, kPanMax), kPanMin, kPanMax);
}

double ptz_tilt_to_onvif(int tilt) {
    return angle_to_onvif(clamp_int(tilt, kTiltMin, kTiltMax), kTiltMin, kTiltMax);
}

std::string read_ptz_position(const Config& config, PtzPosition& out) {
    PtzStatus status{};
    const std::string error = read_ptz_status(config, status);
    if (error.empty()) {
        out = status.position;
    }
    return error;
}

std::string read_ptz_status(const Config& config, PtzStatus& out) {
    if (config.ptz_dry_run) {
        out.position = {g_dry_run_pan.load(), g_dry_run_tilt.load()};
        out.pan_speed = g_dry_run_pan_speed.load();
        out.tilt_speed = g_dry_run_tilt_speed.load();
        return {};
    }

    const int fd = open(config.ptz_device.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return "failed to open PTZ device " + config.ptz_device + ": " + std::strerror(errno);
    }

    std::string response;
    char buffer[128];
    while (true) {
        const ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read < 0) {
            const std::string error = "failed to read PTZ position from " + config.ptz_device + ": " + std::strerror(errno);
            close(fd);
            return error;
        }
        if (bytes_read == 0) {
            break;
        }
        response.append(buffer, static_cast<std::size_t>(bytes_read));
        if (response.find('\n') != std::string::npos) {
            break;
        }
    }

    if (close(fd) != 0) {
        return "failed to close PTZ device " + config.ptz_device + ": " + std::strerror(errno);
    }
    if (!parse_status_text(response, out)) {
        return "failed to parse PTZ status from " + config.ptz_device + ": " + response;
    }
    return {};
}

std::string write_ptz_position(const Config& config, const PtzPosition& position) {
    std::lock_guard<std::mutex> lock(g_command_mutex);
    ++g_motion_generation;
    const PtzPosition clamped = clamp_ptz_position(position);
    const std::string error = write_ptz_command_unlocked(config, position_wire(clamped));
    if (error.empty() && config.ptz_dry_run) {
        g_dry_run_pan = clamped.pan;
        g_dry_run_tilt = clamped.tilt;
        g_dry_run_pan_speed = 0;
        g_dry_run_tilt_speed = 0;
    }
    return error;
}

std::string continuous_move_ptz(const Config& config, double x, double y, int timeout_ms) {
    const int pan_speed = speed_from_axis(config, x);
    const int tilt_speed = speed_from_axis(config, y);

    std::cout << "[info] ptz continuous"
              << " x=" << x
              << " y=" << y
              << " pan_speed=" << pan_speed
              << " tilt_speed=" << tilt_speed
              << "\n";

    unsigned long long generation = 0;
    {
        std::lock_guard<std::mutex> lock(g_command_mutex);
        generation = ++g_motion_generation;
        const std::string error = write_ptz_command_unlocked(config, speed_wire(pan_speed, tilt_speed));
        if (!error.empty()) {
            return error;
        }
        if (config.ptz_dry_run) {
            g_dry_run_pan_speed = pan_speed;
            g_dry_run_tilt_speed = tilt_speed;
        }
    }

    std::thread([config, generation, timeout_ms] {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
        std::lock_guard<std::mutex> lock(g_command_mutex);
        if (g_motion_generation.load() != generation) {
            return;
        }
        const std::string error = write_ptz_command_unlocked(config, "stop\n");
        if (error.empty()) {
            if (config.ptz_dry_run) {
                g_dry_run_pan_speed = 0;
                g_dry_run_tilt_speed = 0;
            }
            std::cout << "[info] ptz timeout stop after " << timeout_ms << "ms\n";
        } else {
            std::cerr << "[error] ptz timeout stop failed: " << error << "\n";
        }
        ++g_motion_generation;
    }).detach();
    return {};
}

std::string relative_move_ptz(const Config& config, double x, double y) {
    return move_ptz_by_axes(config, x, y, "relative");
}

std::string stop_ptz(const Config& config) {
    std::cout << "[info] ptz stop\n";
    std::lock_guard<std::mutex> lock(g_command_mutex);
    ++g_motion_generation;
    const std::string error = write_ptz_command_unlocked(config, "stop\n");
    if (error.empty() && config.ptz_dry_run) {
        g_dry_run_pan_speed = 0;
        g_dry_run_tilt_speed = 0;
    }
    return error;
}

std::string home_ptz(const Config& config) {
    return write_ptz_position(config, home_ptz_position());
}

std::string set_home_ptz_from_current(const Config& config) {
    PtzPosition position{};
    const std::string error = read_ptz_position(config, position);
    if (!error.empty()) {
        return error;
    }

    std::lock_guard<std::mutex> lock(g_home_mutex);
    g_home_position = clamp_ptz_position(position);
    return {};
}

}  // namespace afterveda_onvif
