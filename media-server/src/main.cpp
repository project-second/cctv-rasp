#include "config.h"
#include "control_server.h"
#include "media_server.h"
#include "pipeline.h"

#include <gst/gst.h>

#include <iostream>
#include <memory>
#include <stdexcept>

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);

    try {
        const rail_media::Config config = rail_media::parse_args(argc, argv);
        const std::string launch = rail_media::build_launch_pipeline(config);

        std::unique_ptr<GMainLoop, decltype(&g_main_loop_unref)> loop(
            g_main_loop_new(nullptr, FALSE),
            g_main_loop_unref);
        rail_media::MediaServer media_server(config);
        rail_media::ControlServer control_server(media_server, config.control_port);

        std::cout << "[info] rail-media started\n"
                  << "[info]   url: rtsp://<pi-ip>:" << config.port << config.mount << "\n"
                  << "[info]   control: http://<pi-ip>:" << config.control_port << "\n"
                  << "[info]   profile: " << config.profile << "\n"
                  << "[info]   resolution: " << config.width << "x" << config.height << "\n"
                  << "[info]   fps: " << config.fps << "\n"
                  << "[info]   bitrate: " << config.bitrate_kbps << " kbps\n"
                  << "[info]   encoder: " << config.encoder << "\n"
                  << "[info]   pipeline: " << launch << "\n";

        g_main_loop_run(loop.get());
    } catch (const std::exception& error) {
        std::cerr << "[error] rail-media: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
