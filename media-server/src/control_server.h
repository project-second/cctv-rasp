#pragma once

#include "media_server.h"

#include <string>
#include <vector>

#include <gio/gio.h>

namespace rail_media {

class ControlServer {
public:
    ControlServer(MediaServer& media_server, const std::string& port);
    ~ControlServer();

    ControlServer(const ControlServer&) = delete;
    ControlServer& operator=(const ControlServer&) = delete;

private:
    static gboolean on_connection(GSocketService* service, GSocketConnection* connection, GObject* source_object, gpointer user_data);

    std::string handle_request(const std::string& request);

    MediaServer& media_server_;
    GSocketService* service_ = nullptr;
};

std::string profile_json(const Config& config);
std::string profiles_json();
std::string http_response(int status, const std::string& reason, const std::string& body);
std::string json_escape(const std::string& value);
std::vector<std::string> split_request_line(const std::string& line);

}  // namespace rail_media
