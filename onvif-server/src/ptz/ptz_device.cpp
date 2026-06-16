#include "ptz_device.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <string>
#include <unistd.h>

namespace afterveda_onvif {
namespace {

constexpr int kPanMin = 30;
constexpr int kPanMax = 150;
constexpr int kTiltMin = 45;
constexpr int kTiltMax = 135;
constexpr int kHomePan = 90;
constexpr int kHomeTilt = 45;

std::mutex g_home_mutex;
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

bool parse_position_text(const std::string& text, PtzPosition& out) {
    int pan = 0;
    int tilt = 0;
    if (!parse_int_after(text, "pan=", pan) || !parse_int_after(text, "tilt=", tilt)) {
        return false;
    }
    out = clamp_ptz_position({pan, tilt});
    return true;
}

bool parse_double_text(const std::string& text, double& out) {
    char* end = nullptr;
    const double value = std::strtod(text.c_str(), &end);
    if (text.c_str() == end) {
        return false;
    }
    out = value;
    return true;
}

bool extract_between(const std::string& request, const std::string& begin_marker, const std::string& end_marker, double& out) {
    const std::size_t begin = request.find(begin_marker);
    if (begin == std::string::npos) {
        return false;
    }
    const std::size_t value_begin = begin + begin_marker.size();
    const std::size_t end = request.find(end_marker, value_begin);
    if (end == std::string::npos) {
        return false;
    }
    return parse_double_text(request.substr(value_begin, end - value_begin), out);
}

double extract_axis_value(const std::string& request, char axis) {
    double value = 0.0;
    const std::string attr_marker = std::string(1, axis) + "=\"";
    if (extract_between(request, attr_marker, "\"", value)) {
        return value;
    }
    if (extract_between(request, std::string("<tt:") + axis + ">", std::string("</tt:") + axis + ">", value)) {
        return value;
    }
    if (extract_between(request, std::string("<") + axis + ">", std::string("</") + axis + ">", value)) {
        return value;
    }
    return 0.0;
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
    if (config.ptz_dry_run) {
        out = home_ptz_position();
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
    if (!parse_position_text(response, out)) {
        return "failed to parse PTZ position from " + config.ptz_device + ": " + response;
    }
    return {};
}

std::string write_ptz_position(const Config& config, const PtzPosition& position) {
    const std::string wire = position_wire(position);
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

std::string move_ptz_from_request(const Config& config, const std::string& request) {
    PtzPosition position{};
    const std::string error = read_ptz_position(config, position);
    if (!error.empty()) {
        return error;
    }

    position.pan += delta_from_axis(config, extract_axis_value(request, 'x'));
    position.tilt += delta_from_axis(config, extract_axis_value(request, 'y'));
    return write_ptz_position(config, clamp_ptz_position(position));
}

std::string stop_ptz(const Config& config) {
    PtzPosition position{};
    const std::string error = read_ptz_position(config, position);
    if (!error.empty()) {
        return error;
    }
    return write_ptz_position(config, position);
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
