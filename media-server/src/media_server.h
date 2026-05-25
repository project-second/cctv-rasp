#pragma once

#include "config.h"

#include <condition_variable>
#include <mutex>
#include <string>
#include <utility>

#include <glib.h>
#include <gst/rtsp-server/rtsp-server.h>

namespace rail_media {

class MediaServer {
public:
    explicit MediaServer(const Config& config);
    ~MediaServer();

    MediaServer(const MediaServer&) = delete;
    MediaServer& operator=(const MediaServer&) = delete;

    Config current_config() const;
    std::pair<bool, std::string> switch_profile(const std::string& profile);

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

    static gboolean switch_profile_on_main(gpointer user_data);
    static GstRTSPFilterResult close_rtsp_client(GstRTSPServer* server, GstRTSPClient* client, gpointer user_data);

    void configure_auth(const Config& config);
    GstRTSPMediaFactory* create_factory(const Config& config) const;
    void install_factory(const Config& config);
    std::pair<bool, std::string> switch_profile_now(const std::string& profile);

    GstRTSPServer* server_ = nullptr;
    guint source_id_ = 0;
    std::string mount_;
    Config current_;
    mutable std::mutex mutex_;
};

}  // namespace rail_media
