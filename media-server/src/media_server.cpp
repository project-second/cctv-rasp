#include "media_server.h"

#include "control_server.h"
#include "pipeline.h"
#include "profiles.h"

#include <iostream>
#include <stdexcept>

namespace rail_media {

namespace {

constexpr const char* kRtspViewerRole = "viewer";

bool rtsp_auth_enabled(const Config& config) {
    return !config.rtsp_user.empty() && !config.rtsp_password.empty();
}

}  // namespace

MediaServer::MediaServer(const Config& config)
    : mount_(config.mount),
      current_(config) {
    server_ = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server_, config.port.c_str());
    configure_auth(config);
    install_factory(config);

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

std::pair<bool, std::string> MediaServer::switch_profile(const std::string& profile) {
    ProfileSwitchRequest request;
    request.server = this;
    request.profile = profile;

    g_idle_add(switch_profile_on_main, &request);

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

GstRTSPFilterResult MediaServer::close_rtsp_client(GstRTSPServer*, GstRTSPClient* client, gpointer) {
    gst_rtsp_client_close(client);
    return GST_RTSP_FILTER_REMOVE;
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

GstRTSPMediaFactory* MediaServer::create_factory(const Config& config) const {
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
    return factory;
}

void MediaServer::install_factory(const Config& config) {
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

        install_factory(next);
        gst_rtsp_server_client_filter(server_, close_rtsp_client, nullptr);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            current_ = next;
        }

        std::cout << "[info] profile switched\n"
                  << "[info]   profile: " << next.profile << "\n"
                  << "[info]   resolution: " << next.width << "x" << next.height << "\n"
                  << "[info]   fps: " << next.fps << "\n"
                  << "[info]   bitrate: " << next.bitrate_kbps << " kbps\n"
                  << "[info]   pipeline: " << build_launch_pipeline(next) << "\n";

        return {true, profile_json(next)};
    } catch (const std::exception& error) {
        std::cerr << "[error] profile switch failed: " << error.what() << "\n";
        return {false, std::string("{\"error\":\"") + json_escape(error.what()) + "\"}\n"};
    }
}

}  // namespace rail_media
