#include "pipeline.h"

#include <sstream>
#include <string>

namespace rail_media {

namespace {

double balance_brightness(double value) {
    return (value - 50.0) / 50.0;
}

double balance_multiplier(double value) {
    return value / 50.0;
}

std::string video_balance_pipeline(const Config& config) {
    std::ostringstream out;
    out << "videobalance brightness=" << balance_brightness(config.brightness)
        << " contrast=" << balance_multiplier(config.contrast)
        << " saturation=" << balance_multiplier(config.color_saturation);
    return out.str();
}

std::string gst_string(const std::string& value) {
    std::string quoted = "\"";
    for (const char c : value) {
        if (c == '\\' || c == '"') {
            quoted += '\\';
        }
        quoted += c;
    }
    quoted += '"';
    return quoted;
}

std::string osd_pipeline(const Config& config) {
    if (!config.osd_enabled || config.osd_text.empty()) {
        return "";
    }

    return "! videoconvert "
        "! textoverlay text=" + gst_string(config.osd_text)
        + " valignment=top halignment=left shaded-background=true font-desc=" + gst_string("Sans, 24") + " "
        "! videoconvert ! video/x-raw,format=NV12 ";
}

}  // namespace

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
        "! " + video_balance_pipeline(config) + " "
        + osd_pipeline(config)
        + "! queue "
        "! " + encoder_pipeline(config) + " "
        "! rtph264pay config-interval=1 name=pay0 pt=96 )";
}

}  // namespace rail_media
