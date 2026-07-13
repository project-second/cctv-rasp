#include "media_server.h"

#include "control_server.h"
#include "pipeline.h"
#include "profiles.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace rail_media {

namespace {

constexpr const char* kRtspViewerRole = "viewer";

bool rtsp_auth_enabled(const Config& config) {
    return !config.rtsp_user.empty() && !config.rtsp_password.empty();
}

bool supported_dynamic_video_config(const Config& config) {
    const bool supported_resolution =
        (config.width == 640 && config.height == 360) ||
        (config.width == 1280 && config.height == 720) ||
        (config.width == 1920 && config.height == 1080);
    return supported_resolution && config.fps == 30 &&
        config.bitrate_kbps >= 800 && config.bitrate_kbps <= 5000;
}

const char* pipeline_status_name(GstRTSPMediaStatus status) {
    switch (status) {
    case GST_RTSP_MEDIA_STATUS_UNPREPARED:
        return "idle";
    case GST_RTSP_MEDIA_STATUS_UNPREPARING:
        return "stopping";
    case GST_RTSP_MEDIA_STATUS_PREPARING:
        return "preparing";
    case GST_RTSP_MEDIA_STATUS_PREPARED:
        return "ready";
    case GST_RTSP_MEDIA_STATUS_SUSPENDED:
        return "suspended";
    case GST_RTSP_MEDIA_STATUS_ERROR:
        return "error";
    }
    return "unknown";
}

std::string utc_timestamp(std::time_t value) {
    if (value == 0) {
        return {};
    }
    std::tm timestamp{};
    gmtime_r(&value, &timestamp);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timestamp);
    return buffer;
}

}  // namespace

MediaServer::MediaServer(const Config& config)
    : mount_(config.mount),
      current_(config) {
    current_.profile = "main";
    server_ = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server_, config.port.c_str());
    g_signal_connect(server_, "client-connected", G_CALLBACK(on_client_connected), this);
    configure_auth(config);
    install_factory(current_);

    source_id_ = gst_rtsp_server_attach(server_, nullptr);
    if (source_id_ == 0) {
        throw std::runtime_error("failed to attach RTSP server");
    }
}

MediaServer::~MediaServer() {
    if (source_id_ != 0) {
        g_source_remove(source_id_);
    }
    if (server_ != nullptr) {
        g_object_unref(server_);
    }
}

Config MediaServer::current_config() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_;
}

HealthStatus MediaServer::health_status() const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - started_monotonic_);
    return {
        source_id_ != 0,
        pipeline_status_,
        camera_status_,
        utc_timestamp(started_at_),
        utc_timestamp(last_pipeline_ready_at_),
        last_error_,
        uptime.count(),
        rtsp_clients_,
    };
}

std::pair<bool, std::string> MediaServer::switch_profile(const std::string& profile) {
    ProfileSwitchRequest request;
    request.server = this;
    request.profile = profile;

    g_idle_add(switch_profile_on_main, &request);

    std::unique_lock<std::mutex> lock(request.mutex);
    request.cv.wait(lock, [&] { return request.done; });
    return {request.ok, request.message};
}

std::pair<bool, std::string> MediaServer::set_video_config(const Config& next) {
    VideoConfigRequest request;
    request.server = this;
    request.next = next;

    g_idle_add(set_video_config_on_main, &request);

    std::unique_lock<std::mutex> lock(request.mutex);
    request.cv.wait(lock, [&] { return request.done; });
    return {request.ok, request.message};
}

std::pair<bool, std::string> MediaServer::set_imaging(double brightness, double contrast, double color_saturation) {
    ImagingRequest request;
    request.server = this;
    request.brightness = brightness;
    request.contrast = contrast;
    request.color_saturation = color_saturation;

    g_idle_add(set_imaging_on_main, &request);

    std::unique_lock<std::mutex> lock(request.mutex);
    request.cv.wait(lock, [&] { return request.done; });
    return {request.ok, request.message};
}

std::pair<bool, std::string> MediaServer::set_osd(bool enabled, const std::string& text) {
    OsdRequest request;
    request.server = this;
    request.enabled = enabled;
    request.text = text;

    g_idle_add(set_osd_on_main, &request);

    std::unique_lock<std::mutex> lock(request.mutex);
    request.cv.wait(lock, [&] { return request.done; });
    return {request.ok, request.message};
}

gboolean MediaServer::switch_profile_on_main(gpointer user_data) {
    auto* request = static_cast<ProfileSwitchRequest*>(user_data);
    const auto [ok, message] = request->server->switch_profile_now(request->profile);

    request->ok = ok;
    request->message = message;

    {
        std::lock_guard<std::mutex> lock(request->mutex);
        request->done = true;
    }
    request->cv.notify_one();
    return G_SOURCE_REMOVE;
}

gboolean MediaServer::set_video_config_on_main(gpointer user_data) {
    auto* request = static_cast<VideoConfigRequest*>(user_data);
    const auto [ok, message] = request->server->set_video_config_now(request->next);

    request->ok = ok;
    request->message = message;

    {
        std::lock_guard<std::mutex> lock(request->mutex);
        request->done = true;
    }
    request->cv.notify_one();
    return G_SOURCE_REMOVE;
}

gboolean MediaServer::set_imaging_on_main(gpointer user_data) {
    auto* request = static_cast<ImagingRequest*>(user_data);
    const auto [ok, message] = request->server->set_imaging_now(
        request->brightness,
        request->contrast,
        request->color_saturation);

    request->ok = ok;
    request->message = message;

    {
        std::lock_guard<std::mutex> lock(request->mutex);
        request->done = true;
    }
    request->cv.notify_one();
    return G_SOURCE_REMOVE;
}

gboolean MediaServer::set_osd_on_main(gpointer user_data) {
    auto* request = static_cast<OsdRequest*>(user_data);
    const auto [ok, message] = request->server->set_osd_now(request->enabled, request->text);

    request->ok = ok;
    request->message = message;

    {
        std::lock_guard<std::mutex> lock(request->mutex);
        request->done = true;
    }
    request->cv.notify_one();
    return G_SOURCE_REMOVE;
}

GstRTSPFilterResult MediaServer::close_rtsp_client(GstRTSPServer*, GstRTSPClient* client, gpointer) {
    gst_rtsp_client_close(client);
    return GST_RTSP_FILTER_REMOVE;
}

void MediaServer::on_media_configure(GstRTSPMediaFactory*, GstRTSPMedia* media, gpointer user_data) {
    auto* server = static_cast<MediaServer*>(user_data);
    server->update_pipeline_status("preparing", "checking");
    g_signal_connect(media, "prepared", G_CALLBACK(on_media_prepared), server);
    g_signal_connect(media, "unprepared", G_CALLBACK(on_media_unprepared), server);
    g_signal_connect(media, "new-state", G_CALLBACK(on_media_new_state), server);

    GstElement* element = gst_rtsp_media_get_element(media);
    if (element != nullptr) {
        GstBus* bus = gst_element_get_bus(element);
        if (bus != nullptr) {
            gst_bus_enable_sync_message_emission(bus);
            g_signal_connect(bus, "sync-message", G_CALLBACK(on_bus_message), server);
            gst_object_unref(bus);
        }
        gst_object_unref(element);
    }
}

void MediaServer::on_media_prepared(GstRTSPMedia*, gpointer user_data) {
    auto* server = static_cast<MediaServer*>(user_data);
    {
        std::lock_guard<std::mutex> lock(server->mutex_);
        server->pipeline_status_ = "ready";
        server->camera_status_ = "ok";
        server->last_pipeline_ready_at_ = std::time(nullptr);
    }
}

void MediaServer::on_media_unprepared(GstRTSPMedia* media, gpointer user_data) {
    auto* server = static_cast<MediaServer*>(user_data);
    const GstRTSPMediaStatus status = gst_rtsp_media_get_status(media);
    if (status == GST_RTSP_MEDIA_STATUS_ERROR) {
        server->update_pipeline_status("error", "error");
        return;
    }
    if (server->health_status().pipeline_status == "error") {
        return;
    }
    server->update_pipeline_status(pipeline_status_name(status), "not_checked");
}

void MediaServer::on_media_new_state(GstRTSPMedia* media, GstState, gpointer user_data) {
    auto* server = static_cast<MediaServer*>(user_data);
    if (gst_rtsp_media_get_status(media) == GST_RTSP_MEDIA_STATUS_ERROR) {
        server->update_pipeline_status("error", "error");
    }
}

void MediaServer::on_bus_message(GstBus*, GstMessage* message, gpointer user_data) {
    auto* server = static_cast<MediaServer*>(user_data);
    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
        GError* error = nullptr;
        gchar* debug = nullptr;
        gst_message_parse_error(message, &error, &debug);

        std::ostringstream description;
        if (GST_MESSAGE_SRC(message) != nullptr) {
            description << GST_OBJECT_NAME(GST_MESSAGE_SRC(message)) << ": ";
        }
        description << (error != nullptr ? error->message : "unknown GStreamer error");
        {
            std::lock_guard<std::mutex> lock(server->mutex_);
            server->pipeline_status_ = "error";
            server->camera_status_ = "error";
            server->last_error_ = description.str();
        }
        std::cerr << "[error] pipeline: " << description.str() << "\n";
        if (error != nullptr) {
            g_error_free(error);
        }
        g_free(debug);
    } else if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS) {
        std::lock_guard<std::mutex> lock(server->mutex_);
        server->pipeline_status_ = "error";
        server->camera_status_ = "error";
        server->last_error_ = "GStreamer pipeline reached end of stream";
    }
}

void MediaServer::on_client_connected(GstRTSPServer*, GstRTSPClient* client, gpointer user_data) {
    auto* server = static_cast<MediaServer*>(user_data);
    {
        std::lock_guard<std::mutex> lock(server->mutex_);
        ++server->rtsp_clients_;
    }
    g_signal_connect(client, "closed", G_CALLBACK(on_client_closed), server);
}

void MediaServer::on_client_closed(GstRTSPClient*, gpointer user_data) {
    auto* server = static_cast<MediaServer*>(user_data);
    std::lock_guard<std::mutex> lock(server->mutex_);
    if (server->rtsp_clients_ > 0) {
        --server->rtsp_clients_;
    }
}

void MediaServer::update_pipeline_status(std::string pipeline_status, std::string camera_status) {
    std::lock_guard<std::mutex> lock(mutex_);
    pipeline_status_ = std::move(pipeline_status);
    camera_status_ = std::move(camera_status);
}

void MediaServer::configure_auth(const Config& config) {
    if (!rtsp_auth_enabled(config)) {
        return;
    }

    GstRTSPAuth* auth = gst_rtsp_auth_new();
    gst_rtsp_auth_set_realm(auth, config.rtsp_realm.c_str());
    gst_rtsp_auth_set_supported_methods(auth, GST_RTSP_AUTH_DIGEST);

    GstRTSPToken* token = gst_rtsp_token_new(
        GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE,
        G_TYPE_STRING,
        kRtspViewerRole,
        nullptr);
    gst_rtsp_auth_add_digest(auth, config.rtsp_user.c_str(), config.rtsp_password.c_str(), token);
    gst_rtsp_token_unref(token);

    gst_rtsp_server_set_auth(server_, auth);
    g_object_unref(auth);
}

GstRTSPMediaFactory* MediaServer::create_factory(const Config& config) {
    GstRTSPMediaFactory* factory = gst_rtsp_media_factory_new();
    const std::string launch = build_launch_pipeline(config);
    gst_rtsp_media_factory_set_launch(factory, launch.c_str());
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    if (rtsp_auth_enabled(config)) {
        gst_rtsp_media_factory_add_role(
            factory,
            kRtspViewerRole,
            GST_RTSP_PERM_MEDIA_FACTORY_ACCESS,
            G_TYPE_BOOLEAN,
            TRUE,
            GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT,
            G_TYPE_BOOLEAN,
            TRUE,
            nullptr);
    }
    g_signal_connect(factory, "media-configure", G_CALLBACK(on_media_configure), this);
    return factory;
}

void MediaServer::install_factory(const Config& config) {
    update_pipeline_status("idle", "not_checked");
    GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points(server_);
    gst_rtsp_mount_points_remove_factory(mounts, mount_.c_str());
    gst_rtsp_mount_points_add_factory(mounts, mount_.c_str(), create_factory(config));
    g_object_unref(mounts);
}

std::pair<bool, std::string> MediaServer::switch_profile_now(const std::string& profile_name) {
    try {
        const Profile* profile = find_profile(profile_name);
        if (profile == nullptr) {
            throw std::runtime_error("unknown profile: " + profile_name);
        }

        Config next;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            next = current_;
        }
        apply_profile(next, *profile);
        // low/main/high are local quality presets for the single stable ONVIF
        // profile, not separate media profile identities.
        next.profile = "main";

        install_factory(next);
        gst_rtsp_server_client_filter(server_, close_rtsp_client, nullptr);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            current_ = next;
        }

        std::cout << "[info] video preset applied\n"
                  << "[info]   preset: " << profile_name << "\n"
                  << "[info]   profile: " << next.profile << "\n"
                  << "[info]   resolution: " << next.width << "x" << next.height << "\n"
                  << "[info]   fps: " << next.fps << "\n"
                  << "[info]   bitrate: " << next.bitrate_kbps << " kbps\n"
                  << "[info]   pipeline: " << build_launch_pipeline(next) << "\n";

        return {true, profile_json(next)};
    } catch (const std::exception& error) {
        std::cerr << "[error] video preset failed: " << error.what() << "\n";
        return {false, std::string("{\"error\":\"") + json_escape(error.what()) + "\"}\n"};
    }
}

std::pair<bool, std::string> MediaServer::set_video_config_now(const Config& next_config) {
    try {
        Config next = next_config;
        if (next.width <= 0 || next.height <= 0 || next.fps <= 0 || next.bitrate_kbps <= 0) {
            throw std::runtime_error("video config values must be positive");
        }
        if (!supported_dynamic_video_config(next)) {
            throw std::runtime_error(
                "supported video settings are 640x360, 1280x720, or 1920x1080 at 30 fps and 800..5000 kbps");
        }
        if (next.encoder != "v4l2" && next.encoder != "x264") {
            throw std::runtime_error("encoder must be v4l2 or x264");
        }

        install_factory(next);
        gst_rtsp_server_client_filter(server_, close_rtsp_client, nullptr);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            current_ = next;
        }

        std::cout << "[info] video config changed\n"
                  << "[info]   profile: " << next.profile << "\n"
                  << "[info]   resolution: " << next.width << "x" << next.height << "\n"
                  << "[info]   fps: " << next.fps << "\n"
                  << "[info]   bitrate: " << next.bitrate_kbps << " kbps\n"
                  << "[info]   encoder: " << next.encoder << "\n"
                  << "[info]   pipeline: " << build_launch_pipeline(next) << "\n";

        return {true, profile_json(next)};
    } catch (const std::exception& error) {
        std::cerr << "[error] video config update failed: " << error.what() << "\n";
        return {false, std::string("{\"error\":\"") + json_escape(error.what()) + "\"}\n"};
    }
}

std::pair<bool, std::string> MediaServer::set_imaging_now(double brightness, double contrast, double color_saturation) {
    try {
        Config next;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            next = current_;
        }
        next.brightness = brightness;
        next.contrast = contrast;
        next.color_saturation = color_saturation;

        install_factory(next);
        gst_rtsp_server_client_filter(server_, close_rtsp_client, nullptr);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            current_ = next;
        }

        std::cout << "[info] imaging settings changed\n"
                  << "[info]   brightness: " << next.brightness << "\n"
                  << "[info]   contrast: " << next.contrast << "\n"
                  << "[info]   color_saturation: " << next.color_saturation << "\n"
                  << "[info]   pipeline: " << build_launch_pipeline(next) << "\n";

        return {true, imaging_json(next)};
    } catch (const std::exception& error) {
        std::cerr << "[error] imaging update failed: " << error.what() << "\n";
        return {false, std::string("{\"error\":\"") + json_escape(error.what()) + "\"}\n"};
    }
}

std::pair<bool, std::string> MediaServer::set_osd_now(bool enabled, const std::string& text) {
    try {
        if (text.size() > 128) {
            throw std::runtime_error("osd text must be 128 characters or fewer");
        }

        Config next;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            next = current_;
        }
        next.osd_enabled = enabled;
        next.osd_text = text;

        install_factory(next);
        gst_rtsp_server_client_filter(server_, close_rtsp_client, nullptr);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            current_ = next;
        }

        std::cout << "[info] osd settings changed\n"
                  << "[info]   enabled: " << (next.osd_enabled ? "true" : "false") << "\n"
                  << "[info]   text: " << next.osd_text << "\n"
                  << "[info]   pipeline: " << build_launch_pipeline(next) << "\n";

        return {true, osd_json(next)};
    } catch (const std::exception& error) {
        std::cerr << "[error] osd update failed: " << error.what() << "\n";
        return {false, std::string("{\"error\":\"") + json_escape(error.what()) + "\"}\n"};
    }
}

}  // namespace rail_media
