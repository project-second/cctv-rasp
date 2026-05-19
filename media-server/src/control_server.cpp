#include "control_server.h"

#include "config.h"
#include "profiles.h"

#include <iostream>
#include <stdexcept>

namespace rail_media {

std::string json_escape(const std::string& value) {
    std::string escaped;
    for (const char c : value) {
        if (c == '"' || c == '\\') {
            escaped += '\\';
        }
        escaped += c;
    }
    return escaped;
}

ControlServer::ControlServer(MediaServer& media_server, const std::string& port)
    : media_server_(media_server) {
    service_ = g_threaded_socket_service_new(8);
    GError* error = nullptr;
    const int parsed_port = parse_int(port, "--control-port");

    if (!g_socket_listener_add_inet_port(
            G_SOCKET_LISTENER(service_),
            static_cast<guint16>(parsed_port),
            nullptr,
            &error)) {
        const std::string message = error != nullptr ? error->message : "unknown error";
        if (error != nullptr) {
            g_error_free(error);
        }
        g_object_unref(service_);
        service_ = nullptr;
        std::cerr << "[error] failed to bind HTTP control port: " << message << "\n";
        throw std::runtime_error("failed to bind HTTP control port: " + message);
    }

    g_signal_connect(service_, "run", G_CALLBACK(on_connection), this);
    g_socket_service_start(service_);
}

ControlServer::~ControlServer() {
    if (service_ != nullptr) {
        g_socket_service_stop(service_);
        g_object_unref(service_);
    }
}

gboolean ControlServer::on_connection(GSocketService*, GSocketConnection* connection, GObject*, gpointer user_data) {
    auto* control_server = static_cast<ControlServer*>(user_data);
    GInputStream* input = g_io_stream_get_input_stream(G_IO_STREAM(connection));
    GOutputStream* output = g_io_stream_get_output_stream(G_IO_STREAM(connection));

    char buffer[4096];
    GError* error = nullptr;
    const gssize read_size = g_input_stream_read(input, buffer, sizeof(buffer) - 1, nullptr, &error);
    if (read_size < 0) {
        if (error != nullptr) {
            std::cerr << "[error] rail-media control: " << error->message << "\n";
            g_error_free(error);
        }
        return TRUE;
    }

    buffer[read_size] = '\0';
    const std::string response = control_server->handle_request(buffer);
    if (!g_output_stream_write_all(output, response.data(), response.size(), nullptr, nullptr, &error)) {
        if (error != nullptr) {
            std::cerr << "[error] rail-media control: " << error->message << "\n";
            g_error_free(error);
        }
    }

    return TRUE;
}

std::string ControlServer::handle_request(const std::string& request) {
    const std::size_t line_end = request.find("\r\n");
    const std::string request_line = request.substr(0, line_end);
    const std::vector<std::string> parts = split_request_line(request_line);
    if (parts.size() < 2) {
        std::cerr << "[error] bad request\n";
        return http_response(400, "Bad Request", "{\"error\":\"bad request\"}\n");
    }

    const std::string& method = parts[0];
    const std::string& path = parts[1];

    if (method == "GET" && path == "/health") {
        return http_response(200, "OK", "{\"status\":\"ok\"}\n");
    }
    if (method == "GET" && path == "/profiles") {
        return http_response(200, "OK", profiles_json());
    }
    if (method == "GET" && path == "/profile") {
        return http_response(200, "OK", profile_json(media_server_.current_config()));
    }
    if (method == "POST" && path.rfind("/profile/", 0) == 0) {
        const std::string profile = path.substr(std::string("/profile/").size());
        const auto [ok, body] = media_server_.switch_profile(profile);
        if (!ok) {
            std::cerr << "[error] profile request failed: " << profile << "\n";
        }
        return ok ? http_response(200, "OK", body)
                  : http_response(404, "Not Found", body);
    }

    std::cerr << "[error] not found: " << method << " " << path << "\n";
    return http_response(404, "Not Found", "{\"error\":\"not found\"}\n");
}

std::string profile_json(const Config& config) {
    return "{\n"
        "  \"profile\": \"" + json_escape(config.profile) + "\",\n"
        "  \"width\": " + std::to_string(config.width) + ",\n"
        "  \"height\": " + std::to_string(config.height) + ",\n"
        "  \"fps\": " + std::to_string(config.fps) + ",\n"
        "  \"bitrate_kbps\": " + std::to_string(config.bitrate_kbps) + ",\n"
        "  \"encoder\": \"" + json_escape(config.encoder) + "\"\n"
        "}\n";
}

std::string profiles_json() {
    std::string body = "[\n";
    std::size_t index = 0;
    const std::size_t count = profile_count();
    for (const Profile* profile = profiles_begin(); profile != profiles_end(); ++profile, ++index) {
        body += "  {\"profile\":\"" + std::string(profile->name)
            + "\",\"quality\":\"" + profile->quality
            + "\",\"width\":" + std::to_string(profile->width)
            + ",\"height\":" + std::to_string(profile->height)
            + ",\"fps\":" + std::to_string(profile->fps)
            + ",\"bitrate_kbps\":" + std::to_string(profile->bitrate_kbps)
            + ",\"encoder\":\"" + profile->encoder + "\"}";
        body += (index + 1 == count) ? "\n" : ",\n";
    }
    body += "]\n";
    return body;
}

std::string http_response(int status, const std::string& reason, const std::string& body) {
    return "HTTP/1.1 " + std::to_string(status) + " " + reason + "\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n"
        + body;
}

std::vector<std::string> split_request_line(const std::string& line) {
    std::vector<std::string> parts;
    std::string part;
    for (const char c : line) {
        if (c == ' ') {
            if (!part.empty()) {
                parts.push_back(part);
                part.clear();
            }
        } else {
            part += c;
        }
    }
    if (!part.empty()) {
        parts.push_back(part);
    }
    return parts;
}

}  // namespace rail_media
