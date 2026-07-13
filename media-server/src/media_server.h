#pragma once

#include "config.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <ctime>
#include <mutex>
#include <string>
#include <utility>

#include <glib.h>
#include <gst/rtsp-server/rtsp-server.h>

namespace rail_media {

struct HealthStatus {
    bool rtsp_server_ready = false;
    std::string pipeline_status = "idle";
    std::string camera_status = "not_checked";
    std::string started_at;
    std::string last_pipeline_ready_at;
    std::string last_error;
    std::int64_t uptime_seconds = 0;
    std::uint32_t rtsp_clients = 0;
};

class MediaServer {
public:
    explicit MediaServer(const Config& config);
    ~MediaServer();

    MediaServer(const MediaServer&) = delete;
    MediaServer& operator=(const MediaServer&) = delete;

    Config current_config() const;
    HealthStatus health_status() const;
    std::pair<bool, std::string> switch_profile(const std::string& profile);
    std::pair<bool, std::string> set_video_config(const Config& next);
    std::pair<bool, std::string> set_imaging(double brightness, double contrast, double color_saturation);
    std::pair<bool, std::string> set_osd(bool enabled, const std::string& text);

private:
    struct ProfileSwitchRequest {
        MediaServer* server;
        std::string profile;
        bool done = false;
        bool ok = false;
        std::string message;
        std::mutex mutex;
        std::condition_variable cv;
    };

    struct ImagingRequest {
        MediaServer* server;
        double brightness;
        double contrast;
        double color_saturation;
        bool done = false;
        bool ok = false;
        std::string message;
        std::mutex mutex;
        std::condition_variable cv;
    };

    struct VideoConfigRequest {
        MediaServer* server;
        Config next;
        bool done = false;
        bool ok = false;
        std::string message;
        std::mutex mutex;
        std::condition_variable cv;
    };

    struct OsdRequest {
        MediaServer* server;
        bool enabled;
        std::string text;
        bool done = false;
        bool ok = false;
        std::string message;
        std::mutex mutex;
        std::condition_variable cv;
    };

    static gboolean switch_profile_on_main(gpointer user_data);
    static gboolean set_video_config_on_main(gpointer user_data);
    static gboolean set_imaging_on_main(gpointer user_data);
    static gboolean set_osd_on_main(gpointer user_data);
    static GstRTSPFilterResult close_rtsp_client(GstRTSPServer* server, GstRTSPClient* client, gpointer user_data);
    static void on_media_configure(GstRTSPMediaFactory* factory, GstRTSPMedia* media, gpointer user_data);
    static void on_media_prepared(GstRTSPMedia* media, gpointer user_data);
    static void on_media_unprepared(GstRTSPMedia* media, gpointer user_data);
    static void on_media_new_state(GstRTSPMedia* media, GstState state, gpointer user_data);
    static void on_bus_message(GstBus* bus, GstMessage* message, gpointer user_data);
    static void on_client_connected(GstRTSPServer* server, GstRTSPClient* client, gpointer user_data);
    static void on_client_closed(GstRTSPClient* client, gpointer user_data);

    void configure_auth(const Config& config);
    GstRTSPMediaFactory* create_factory(const Config& config);
    void install_factory(const Config& config);
    void update_pipeline_status(std::string pipeline_status, std::string camera_status);
    std::pair<bool, std::string> switch_profile_now(const std::string& profile);
    std::pair<bool, std::string> set_video_config_now(const Config& next);
    std::pair<bool, std::string> set_imaging_now(double brightness, double contrast, double color_saturation);
    std::pair<bool, std::string> set_osd_now(bool enabled, const std::string& text);

    GstRTSPServer* server_ = nullptr;
    guint source_id_ = 0;
    std::string mount_;
    Config current_;
    std::string pipeline_status_ = "idle";
    std::string camera_status_ = "not_checked";
    std::string last_error_;
    std::chrono::steady_clock::time_point started_monotonic_ = std::chrono::steady_clock::now();
    std::time_t started_at_ = std::time(nullptr);
    std::time_t last_pipeline_ready_at_ = 0;
    std::uint32_t rtsp_clients_ = 0;
    mutable std::mutex mutex_;
};

}  // namespace rail_media
