#pragma once

#include "media_server.h"

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include <libsoup/soup.h>

namespace rail_media {

class ControlServer {
public:
    ControlServer(MediaServer& media_server, const std::string& host, const std::string& port);
    ~ControlServer();

    ControlServer(const ControlServer&) = delete;
    ControlServer& operator=(const ControlServer&) = delete;

private:
    static void on_request(SoupServer* server, SoupServerMessage* message, const char* path, GHashTable* query, gpointer user_data);
    static gboolean quit_loop(gpointer user_data);

    void run_server(std::string host, std::string port);
    void signal_startup(std::string error);
    void handle_request(SoupServerMessage* message, const std::string& path);

    MediaServer& media_server_;
    GMainContext* context_ = nullptr;
    GMainLoop* loop_ = nullptr;
    std::thread thread_;
    std::mutex startup_mutex_;
    std::condition_variable startup_cv_;
    bool startup_done_ = false;
    std::string startup_error_;
};

std::string profile_json(const Config& config);
std::string imaging_json(const Config& config);
std::string osd_json(const Config& config);
std::string profiles_json(const Config& config);
std::string presets_json();
std::string json_escape(const std::string& value);

}  // namespace rail_media
