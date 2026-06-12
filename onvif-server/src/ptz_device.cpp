#include "ptz_device.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <unistd.h>

namespace afterveda_onvif {
namespace {

std::string command_wire(const Config& config, const std::string& command) {
    return command + " step=" + std::to_string(config.ptz_step) + "\n";
}

}  // namespace

std::string write_ptz_command(const Config& config, const std::string& command) {
    const std::string wire = command_wire(config, command);
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

}  // namespace afterveda_onvif
