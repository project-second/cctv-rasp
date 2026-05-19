#include "pipeline.h"

namespace rail_media {

std::string encoder_pipeline(const Config& config) {
    if (config.encoder == "v4l2") {
        return "v4l2h264enc extra-controls=\"controls,repeat_sequence_header=1,video_bitrate="
            + std::to_string(config.bitrate_kbps * 1000)
            + "\" "
               "! video/x-h264,level=(string)4 "
               "! h264parse";
    }

    return "x264enc bitrate=" + std::to_string(config.bitrate_kbps)
        + " speed-preset=ultrafast tune=zerolatency key-int-max="
        + std::to_string(config.fps * 2)
        + " threads=1 ! h264parse config-interval=1";
}

std::string build_launch_pipeline(const Config& config) {
    return "( libcamerasrc "
        "! video/x-raw,width=" + std::to_string(config.width)
        + ",height=" + std::to_string(config.height)
        + ",framerate=" + std::to_string(config.fps)
        + "/1,format=NV12,interlace-mode=progressive "
        "! queue "
        "! " + encoder_pipeline(config) + " "
        "! rtph264pay config-interval=1 name=pay0 pt=96 )";
}

}  // namespace rail_media
