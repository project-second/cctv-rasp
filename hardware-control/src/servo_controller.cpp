#include "servo_controller.h"

#include "pca9685.h"

#include <cmath>
#include <iostream>

namespace rail_hardware {

int angle_to_count(const Config& config, int angle) {
    const double pulse_us = config.pulse_min_us +
                            (config.pulse_max_us - config.pulse_min_us) * (angle / 180.0);
    const double period_us = 1000000.0 / config.frequency_hz;
    return static_cast<int>(std::lround((pulse_us / period_us) * 4096.0));
}

void write_servos(const Config& config, const Position& position) {
    const int pan_count = angle_to_count(config, position.pan);
    const int tilt_count = angle_to_count(config, position.tilt);

    if (config.dry_run) {
        std::cout << "[dry-run] pan=" << position.pan << " count=" << pan_count
                  << " channel=" << config.pan_channel << "\n"
                  << "[dry-run] tilt=" << position.tilt << " count=" << tilt_count
                  << " channel=" << config.tilt_channel << "\n";
        return;
    }

    Pca9685 pwm(config.i2c_device, config.address);
    pwm.open_device();
    pwm.initialize(config.frequency_hz);
    pwm.set_pwm(config.pan_channel, 0, pan_count);
    pwm.set_pwm(config.tilt_channel, 0, tilt_count);
}

}  // namespace rail_hardware
