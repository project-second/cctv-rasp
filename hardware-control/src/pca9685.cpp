#include "pca9685.h"

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace rail_hardware {

namespace {

constexpr std::uint8_t MODE1 = 0x00;
constexpr std::uint8_t PRESCALE = 0xFE;
constexpr std::uint8_t LED0_ON_L = 0x06;
constexpr std::uint8_t RESTART = 0x80;
constexpr std::uint8_t SLEEP = 0x10;
constexpr std::uint8_t AI = 0x20;

std::runtime_error system_error(const std::string& message) {
    return std::runtime_error(message + ": " + std::strerror(errno));
}

}  // namespace

Pca9685::Pca9685(std::string device, int address)
    : device_(std::move(device)), address_(address) {}

Pca9685::~Pca9685() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

void Pca9685::open_device() {
    fd_ = open(device_.c_str(), O_RDWR);
    if (fd_ < 0) {
        throw system_error("failed to open " + device_);
    }

    if (ioctl(fd_, I2C_SLAVE, address_) < 0) {
        throw system_error("failed to select i2c address");
    }
}

void Pca9685::initialize(int frequency_hz) {
    if (frequency_hz <= 0) {
        throw std::runtime_error("frequency must be positive");
    }

    const double prescale_value = 25000000.0 / (4096.0 * frequency_hz) - 1.0;
    const auto prescale = static_cast<std::uint8_t>(std::lround(prescale_value));
    const std::uint8_t old_mode = read_register(MODE1);
    const std::uint8_t sleep_mode = (old_mode & static_cast<std::uint8_t>(~RESTART)) | SLEEP;

    write_register(MODE1, sleep_mode);
    write_register(PRESCALE, prescale);
    write_register(MODE1, old_mode);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    write_register(MODE1, old_mode | RESTART | AI);
}

void Pca9685::set_pwm(int channel, int on_count, int off_count) {
    if (channel < 0 || channel > 15) {
        throw std::runtime_error("pwm channel must be between 0 and 15");
    }
    if (on_count < 0 || on_count > 4095 || off_count < 0 || off_count > 4095) {
        throw std::runtime_error("pwm counts must be between 0 and 4095");
    }

    const std::uint8_t base = static_cast<std::uint8_t>(LED0_ON_L + 4 * channel);
    const std::array<std::uint8_t, 5> data{
        base,
        static_cast<std::uint8_t>(on_count & 0xFF),
        static_cast<std::uint8_t>((on_count >> 8) & 0x0F),
        static_cast<std::uint8_t>(off_count & 0xFF),
        static_cast<std::uint8_t>((off_count >> 8) & 0x0F),
    };

    if (write(fd_, data.data(), data.size()) != static_cast<ssize_t>(data.size())) {
        throw system_error("failed to write pwm channel");
    }
}

void Pca9685::write_register(std::uint8_t reg, std::uint8_t value) {
    const std::array<std::uint8_t, 2> data{reg, value};
    if (write(fd_, data.data(), data.size()) != static_cast<ssize_t>(data.size())) {
        throw system_error("failed to write i2c register");
    }
}

std::uint8_t Pca9685::read_register(std::uint8_t reg) {
    if (write(fd_, &reg, 1) != 1) {
        throw system_error("failed to select i2c register");
    }

    std::uint8_t value = 0;
    if (read(fd_, &value, 1) != 1) {
        throw system_error("failed to read i2c register");
    }
    return value;
}

}  // namespace rail_hardware
