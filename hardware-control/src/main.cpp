#include "config.h"
#include "pan_tilt.h"
#include "servo_controller.h"
#include "state_store.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace rail_hardware {

int run(int argc, char* argv[]) {
    const Config config = parse_args(argc, argv);
    const Position current = load_position(config);
    const Position next = apply_command(config, current);

    write_servos(config, next);
    save_position(config, next);

    std::cout << "[info] hardware-control: command=" << config.command
              << " pan=" << next.pan
              << " tilt=" << next.tilt
              << " address=0x" << std::hex << config.address << std::dec
              << "\n";
    return 0;
}

}  // namespace rail_hardware

int main(int argc, char* argv[]) {
    try {
        return rail_hardware::run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "[error] hardware-control: " << error.what() << "\n";
        return 1;
    }
}
