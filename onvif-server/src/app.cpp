#include "app.h"

#include "config.h"
#include "discovery_server.h"
#include "http_server.h"
#include "utils.h"

#include <atomic>
#include <csignal>
#include <exception>
#include <iostream>
#include <thread>

namespace afterveda_onvif {
namespace {

std::atomic<bool> g_running{true};

void handle_signal(int) {
    g_running = false;
}

}  // namespace

int run(int argc, char* argv[]) {
    const Config config = parse_args(argc, argv);
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    std::cout << "[info] afterveda-onvif started\n"
              << "[info]   device: " << config.device_name << "\n"
              << "[info]   xaddr: " << xaddr_base(config) << "/device_service\n"
              << "[info]   rtsp: " << config.rtsp_uri << "\n"
              << "[info]   rail control: " << config.rail_control_url << "\n";

    std::thread discovery([config] {
        try {
            discovery_server(config, g_running);
        } catch (const std::exception& error) {
            std::cerr << "[error] WS-Discovery disabled: " << error.what() << "\n";
        }
    });

    try {
        http_server(config, g_running);
    } catch (...) {
        g_running = false;
        if (discovery.joinable()) {
            discovery.join();
        }
        throw;
    }

    g_running = false;
    if (discovery.joinable()) {
        discovery.join();
    }
    return 0;
}

}  // namespace afterveda_onvif
