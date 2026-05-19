#pragma once

#include <cstdint>
#include <string>

namespace rail_hardware {

class Pca9685 {
public:
    Pca9685(std::string device, int address);
    ~Pca9685();

    Pca9685(const Pca9685&) = delete;
    Pca9685& operator=(const Pca9685&) = delete;

    void open_device();
    void initialize(int frequency_hz);
    void set_pwm(int channel, int on_count, int off_count);

private:
    void write_register(std::uint8_t reg, std::uint8_t value);
    std::uint8_t read_register(std::uint8_t reg);

    std::string device_;
    int address_ = 0;
    int fd_ = -1;
};

}  // namespace rail_hardware
