#include "control_server.h"

#include "config.h"
#include "profiles.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include <json-glib/json-glib.h>

namespace rail_media {

namespace {

JsonObject* parse_json_object(JsonParser* parser, const std::string& body) {
    GError* error = nullptr;
    if (!json_parser_load_from_data(parser, body.data(), static_cast<gssize>(body.size()), &error)) {
        if (error != nullptr) {
            g_error_free(error);
        }
        return nullptr;
    }

    JsonNode* root = json_parser_get_root(parser);
    if (root == nullptr || !JSON_NODE_HOLDS_OBJECT(root)) {
        return nullptr;
    }
    return json_node_get_object(root);
}

bool read_json_int(JsonObject* object, const std::string& key, int& value, bool& present) {
    JsonNode* node = json_object_get_member(object, key.c_str());
    if (node == nullptr) {
        present = false;
        return true;
    }
    present = true;
    if (!JSON_NODE_HOLDS_VALUE(node)) {
        return false;
    }

    int parsed = 0;
    const GType value_type = json_node_get_value_type(node);
    if (value_type == G_TYPE_INT64) {
        const gint64 raw = json_node_get_int(node);
        if (raw <= 0 || raw > std::numeric_limits<int>::max()) {
            return false;
        }
        parsed = static_cast<int>(raw);
    } else if (value_type == G_TYPE_DOUBLE) {
        const double raw = json_node_get_double(node);
        if (!std::isfinite(raw) || raw <= 0.0 || raw > static_cast<double>(std::numeric_limits<int>::max()) || std::floor(raw) != raw) {
            return false;
        }
        parsed = static_cast<int>(raw);
    } else {
        return false;
    }

    value = parsed;
    return true;
}

bool read_json_double(JsonObject* object, const std::string& key, double& value, bool& present) {
    JsonNode* node = json_object_get_member(object, key.c_str());
    if (node == nullptr) {
        present = false;
        return true;
    }
    present = true;
    if (!JSON_NODE_HOLDS_VALUE(node)) {
        return false;
    }

    const GType value_type = json_node_get_value_type(node);
    if (value_type != G_TYPE_INT64 && value_type != G_TYPE_DOUBLE) {
        return false;
    }

    const double parsed = json_node_get_double(node);
    if (!std::isfinite(parsed) || parsed < 0.0 || parsed > 100.0) {
        return false;
    }

    value = parsed;
    return true;
}

bool read_json_bool(JsonObject* object, const std::string& key, bool& value, bool& present) {
    JsonNode* node = json_object_get_member(object, key.c_str());
    if (node == nullptr) {
        present = false;
        return true;
    }
    present = true;
    if (!JSON_NODE_HOLDS_VALUE(node) || json_node_get_value_type(node) != G_TYPE_BOOLEAN) {
        return false;
    }

    value = json_node_get_boolean(node);
    return true;
}

bool read_json_string(JsonObject* object, const std::string& key, std::string& value, bool& present) {
    JsonNode* node = json_object_get_member(object, key.c_str());
    if (node == nullptr) {
        present = false;
        return true;
    }
    present = true;
    if (!JSON_NODE_HOLDS_VALUE(node) || json_node_get_value_type(node) != G_TYPE_STRING) {
        return false;
    }

    const char* parsed = json_node_get_string(node);
    value = parsed == nullptr ? "" : parsed;
    return value.size() <= 128;
}

std::pair<bool, std::string> video_config_from_json(const std::string& body, Config current, Config& next) {
    JsonParser* parser = json_parser_new();
    JsonObject* object = parse_json_object(parser, body);
    if (object == nullptr) {
        g_object_unref(parser);
        return {false, "{\"error\":\"bad video config json\"}\n"};
    }

    next = current;
    bool any = false;
    bool present = false;
    if (!read_json_int(object, "width", next.width, present)) {
        g_object_unref(parser);
        return {false, "{\"error\":\"invalid width\"}\n"};
    }
    any = any || present;
    if (!read_json_int(object, "height", next.height, present)) {
        g_object_unref(parser);
        return {false, "{\"error\":\"invalid height\"}\n"};
    }
    any = any || present;
    if (!read_json_int(object, "fps", next.fps, present)) {
        g_object_unref(parser);
        return {false, "{\"error\":\"invalid fps\"}\n"};
    }
    any = any || present;
    if (!read_json_int(object, "bitrate_kbps", next.bitrate_kbps, present)) {
        g_object_unref(parser);
        return {false, "{\"error\":\"invalid bitrate_kbps\"}\n"};
    }
    any = any || present;

    if (!any) {
        g_object_unref(parser);
        return {false, "{\"error\":\"no video config values\"}\n"};
    }
    // The ONVIF media profile is a stable object. Resolution and rate changes
    // modify its encoder configuration; they must not create a new profile.
    next.profile = "main";
    g_object_unref(parser);
    return {true, ""};
}

std::pair<bool, std::string> imaging_from_json(const std::string& body, Config current, Config& next) {
    JsonParser* parser = json_parser_new();
    JsonObject* object = parse_json_object(parser, body);
    if (object == nullptr) {
        g_object_unref(parser);
        return {false, "{\"error\":\"bad imaging json\"}\n"};
    }

    next = current;
    bool any = false;
    bool present = false;
    if (!read_json_double(object, "brightness", next.brightness, present)) {
        g_object_unref(parser);
        return {false, "{\"error\":\"invalid brightness\"}\n"};
    }
    any = any || present;
    if (!read_json_double(object, "contrast", next.contrast, present)) {
        g_object_unref(parser);
        return {false, "{\"error\":\"invalid contrast\"}\n"};
    }
    any = any || present;
    if (!read_json_double(object, "color_saturation", next.color_saturation, present)) {
        g_object_unref(parser);
        return {false, "{\"error\":\"invalid color_saturation\"}\n"};
    }
    any = any || present;

    if (!any) {
        g_object_unref(parser);
        return {false, "{\"error\":\"no imaging values\"}\n"};
    }
    g_object_unref(parser);
    return {true, ""};
}

std::pair<bool, std::string> osd_from_json(const std::string& body, Config current, Config& next) {
    JsonParser* parser = json_parser_new();
    JsonObject* object = parse_json_object(parser, body);
    if (object == nullptr) {
        g_object_unref(parser);
        return {false, "{\"error\":\"bad osd json\"}\n"};
    }

    next = current;
    bool any = false;
    bool present = false;
    if (!read_json_bool(object, "enabled", next.osd_enabled, present)) {
        g_object_unref(parser);
        return {false, "{\"error\":\"invalid enabled\"}\n"};
    }
    any = any || present;
    if (!read_json_string(object, "text", next.osd_text, present)) {
        g_object_unref(parser);
        return {false, "{\"error\":\"invalid text\"}\n"};
    }
    any = any || present;

    if (!any) {
        g_object_unref(parser);
        return {false, "{\"error\":\"no osd values\"}\n"};
    }
    g_object_unref(parser);
    return {true, ""};
}

std::string request_body(SoupServerMessage* message) {
    SoupMessageBody* body = soup_server_message_get_request_body(message);
    if (body == nullptr || body->data == nullptr || body->length <= 0) {
        return {};
    }
    return {reinterpret_cast<const char*>(body->data), static_cast<std::size_t>(body->length)};
}

void send_json_response(SoupServerMessage* message, guint status, const std::string& body) {
    soup_server_message_set_status(message, status, nullptr);
    soup_server_message_set_response(
        message,
        "application/json",
        SOUP_MEMORY_COPY,
        body.data(),
        body.size());
}

}  // namespace

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

ControlServer::ControlServer(MediaServer& media_server, const std::string& host, const std::string& port)
    : media_server_(media_server) {
    context_ = g_main_context_new();
    loop_ = g_main_loop_new(context_, FALSE);
    if (context_ == nullptr || loop_ == nullptr) {
        if (loop_ != nullptr) {
            g_main_loop_unref(loop_);
            loop_ = nullptr;
        }
        if (context_ != nullptr) {
            g_main_context_unref(context_);
            context_ = nullptr;
        }
        throw std::runtime_error("failed to create HTTP control loop");
    }

    thread_ = std::thread(&ControlServer::run_server, this, host, port);

    {
        std::unique_lock<std::mutex> lock(startup_mutex_);
        startup_cv_.wait(lock, [&] { return startup_done_; });
    }

    if (!startup_error_.empty()) {
        if (thread_.joinable()) {
            thread_.join();
        }
        g_main_loop_unref(loop_);
        g_main_context_unref(context_);
        loop_ = nullptr;
        context_ = nullptr;
        throw std::runtime_error(startup_error_);
    }
}

ControlServer::~ControlServer() {
    if (context_ != nullptr && loop_ != nullptr) {
        g_main_context_invoke(context_, quit_loop, loop_);
    }
    if (thread_.joinable()) {
        thread_.join();
    }
    if (loop_ != nullptr) {
        g_main_loop_unref(loop_);
    }
    if (context_ != nullptr) {
        g_main_context_unref(context_);
    }
}

void ControlServer::on_request(SoupServer*, SoupServerMessage* message, const char* path, GHashTable*, gpointer user_data) {
    auto* control_server = static_cast<ControlServer*>(user_data);
    control_server->handle_request(message, path == nullptr ? "" : path);
}

gboolean ControlServer::quit_loop(gpointer user_data) {
    auto* loop = static_cast<GMainLoop*>(user_data);
    g_main_loop_quit(loop);
    return G_SOURCE_REMOVE;
}

void ControlServer::run_server(std::string host, std::string port) {
    bool startup_reported = false;
    auto report_startup = [&](std::string error) {
        if (!startup_reported) {
            signal_startup(std::move(error));
            startup_reported = true;
        }
    };

    g_main_context_push_thread_default(context_);

    SoupServer* server = soup_server_new("server-header", "rail-media", nullptr);
    if (server == nullptr) {
        report_startup("failed to create HTTP control server");
        g_main_context_pop_thread_default(context_);
        return;
    }

    GError* error = nullptr;
    try {
        const int parsed_port = parse_int(port, "--control-port");
        if (parsed_port > 65535) {
            throw std::runtime_error("invalid --control-port: " + port);
        }
        GInetAddress* inet_address = g_inet_address_new_from_string(host.c_str());
        if (inet_address == nullptr) {
            throw std::runtime_error("invalid --control-host: " + host);
        }
        GSocketAddress* socket_address = g_inet_socket_address_new(inet_address, static_cast<guint16>(parsed_port));
        g_object_unref(inet_address);

        soup_server_add_handler(server, nullptr, on_request, this, nullptr);
        if (!soup_server_listen(
                server,
                socket_address,
                static_cast<SoupServerListenOptions>(0),
                &error)) {
            const std::string message = error != nullptr ? error->message : "unknown error";
            if (error != nullptr) {
                g_error_free(error);
                error = nullptr;
            }
            std::cerr << "[error] failed to bind HTTP control port: " << message << "\n";
            report_startup("failed to bind HTTP control port: " + message);
        } else {
            report_startup({});
            g_main_loop_run(loop_);
        }
        g_object_unref(socket_address);
    } catch (const std::exception& exception) {
        report_startup(exception.what());
    }

    soup_server_disconnect(server);
    g_object_unref(server);
    g_main_context_pop_thread_default(context_);
}

void ControlServer::signal_startup(std::string error) {
    {
        std::lock_guard<std::mutex> lock(startup_mutex_);
        startup_error_ = std::move(error);
        startup_done_ = true;
    }
    startup_cv_.notify_one();
}

void ControlServer::handle_request(SoupServerMessage* message, const std::string& path) {
    const char* method_value = soup_server_message_get_method(message);
    const std::string method = method_value == nullptr ? "" : method_value;

    if (method == "GET" && path == "/health") {
        const HealthStatus health = media_server_.health_status();
        const Config config = media_server_.current_config();
        const bool failed = !health.rtsp_server_ready || health.pipeline_status == "error";
        const std::string status = failed
            ? "error"
            : (health.pipeline_status == "ready" ? "ok" : health.pipeline_status);
        const std::string body =
            "{\n"
            "  \"status\": \"" + status + "\",\n"
            "  \"rtsp_server\": \"" + (health.rtsp_server_ready ? std::string("ready") : std::string("error")) + "\",\n"
            "  \"pipeline\": \"" + json_escape(health.pipeline_status) + "\",\n"
            "  \"camera\": \"" + json_escape(health.camera_status) + "\",\n"
            "  \"profile\": \"" + json_escape(config.profile) + "\",\n"
            "  \"resolution\": \"" + std::to_string(config.width) + "x" + std::to_string(config.height) + "\",\n"
            "  \"fps\": " + std::to_string(config.fps) + ",\n"
            "  \"bitrate_kbps\": " + std::to_string(config.bitrate_kbps) + ",\n"
            "  \"rtsp_clients\": " + std::to_string(health.rtsp_clients) + ",\n"
            "  \"uptime_seconds\": " + std::to_string(health.uptime_seconds) + ",\n"
            "  \"started_at\": \"" + json_escape(health.started_at) + "\",\n"
            "  \"last_pipeline_ready_at\": " + (health.last_pipeline_ready_at.empty()
                ? std::string("null")
                : "\"" + json_escape(health.last_pipeline_ready_at) + "\"") + ",\n"
            "  \"last_error\": " + (health.last_error.empty()
                ? std::string("null")
                : "\"" + json_escape(health.last_error) + "\"") + "\n"
            "}\n";
        send_json_response(message, failed ? 503 : 200, body);
        return;
    }
    if (method == "GET" && path == "/profiles") {
        send_json_response(message, 200, profiles_json(media_server_.current_config()));
        return;
    }
    if (method == "GET" && path == "/presets") {
        send_json_response(message, 200, presets_json());
        return;
    }
    if (method == "GET" && path == "/profile") {
        send_json_response(message, 200, profile_json(media_server_.current_config()));
        return;
    }
    if (method == "GET" && path == "/imaging") {
        send_json_response(message, 200, imaging_json(media_server_.current_config()));
        return;
    }
    if (method == "GET" && path == "/osd") {
        send_json_response(message, 200, osd_json(media_server_.current_config()));
        return;
    }
    if (method == "POST" && path == "/config") {
        Config next;
        const auto [valid, error_body] = video_config_from_json(
            request_body(message),
            media_server_.current_config(),
            next);
        if (!valid) {
            std::cerr << "[error] bad video config request\n";
            send_json_response(message, 400, error_body);
            return;
        }

        const auto [ok, body] = media_server_.set_video_config(next);
        send_json_response(message, ok ? 200 : 500, body);
        return;
    }
    if (method == "POST" &&
        (path.rfind("/preset/", 0) == 0 || path.rfind("/profile/", 0) == 0)) {
        const std::string prefix = path.rfind("/preset/", 0) == 0 ? "/preset/" : "/profile/";
        const std::string preset = path.substr(prefix.size());
        const auto [ok, body] = media_server_.switch_profile(preset);
        if (!ok) {
            std::cerr << "[error] preset request failed: " << preset << "\n";
        }
        send_json_response(message, ok ? 200 : 404, body);
        return;
    }
    if (method == "POST" && path == "/imaging") {
        Config next;
        const auto [valid, error_body] = imaging_from_json(
            request_body(message),
            media_server_.current_config(),
            next);
        if (!valid) {
            std::cerr << "[error] bad imaging request\n";
            send_json_response(message, 400, error_body);
            return;
        }

        const auto [ok, body] = media_server_.set_imaging(
            next.brightness,
            next.contrast,
            next.color_saturation);
        send_json_response(message, ok ? 200 : 500, body);
        return;
    }
    if (method == "POST" && path == "/osd") {
        Config next;
        const auto [valid, error_body] = osd_from_json(
            request_body(message),
            media_server_.current_config(),
            next);
        if (!valid) {
            std::cerr << "[error] bad osd request\n";
            send_json_response(message, 400, error_body);
            return;
        }

        const auto [ok, body] = media_server_.set_osd(next.osd_enabled, next.osd_text);
        send_json_response(message, ok ? 200 : 500, body);
        return;
    }

    std::cerr << "[error] not found: " << method << " " << path << "\n";
    send_json_response(message, 404, "{\"error\":\"not found\"}\n");
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

std::string profiles_json(const Config& config) {
    std::string profile = profile_json(config);
    if (!profile.empty() && profile.back() == '\n') {
        profile.pop_back();
    }
    return "[\n  " + profile + "\n]\n";
}

std::string presets_json() {
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

std::string imaging_json(const Config& config) {
    std::ostringstream out;
    out << "{\"brightness\":" << config.brightness
        << ",\"contrast\":" << config.contrast
        << ",\"color_saturation\":" << config.color_saturation
        << "}\n";
    return out.str();
}

std::string osd_json(const Config& config) {
    return "{\n"
        "  \"enabled\": " + std::string(config.osd_enabled ? "true" : "false") + ",\n"
        "  \"text\": \"" + json_escape(config.osd_text) + "\"\n"
        "}\n";
}

}  // namespace rail_media
