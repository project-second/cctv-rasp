#include "http_server.h"

#include "http_client.h"
#include "soapH.h"

#include <stdsoap2.h>

#include <sys/select.h>
#include <sys/socket.h>

#include <stdexcept>
#include <string>

extern "C" int soap_serve(struct soap*);

namespace afterveda_onvif {
namespace {

int health_check(struct soap* ctx) {
    const auto& config = *static_cast<Config*>(ctx->user);
    const auto rail = http_request_with_status(config.rail_control_url, "GET", "/health");
    const bool rail_healthy = rail.has_value() && rail->status >= 200 && rail->status < 300;
    const int http_status = rail_healthy ? 200 : 503;
    std::string body;
    if (rail.has_value()) {
        const std::string rail_body = rail->body.empty() ? "{}" : rail->body;
        body =
            "{\n"
            "  \"status\": \"" + std::string(rail_healthy ? "ok" : "degraded") + "\",\n"
            "  \"service\": \"afterveda-onvif\",\n"
            "  \"rail_media\": {\n"
            "    \"reachable\": true,\n"
            "    \"http_status\": " + std::to_string(rail->status) + ",\n"
            "    \"health\": " + rail_body +
            "  }\n"
            "}\n";
    } else {
        body =
            "{\n"
            "  \"status\": \"degraded\",\n"
            "  \"service\": \"afterveda-onvif\",\n"
            "  \"rail_media\": {\n"
            "    \"reachable\": false,\n"
            "    \"error\": \"rail-media health endpoint is unavailable\"\n"
            "  }\n"
            "}\n";
    }
    ctx->http_content = "application/json";
    if (soap_response(ctx, SOAP_FILE + http_status) != SOAP_OK) {
        return ctx->error;
    }
    soap_send(ctx, body.c_str());
    return soap_end_send(ctx);
}

}  // namespace

void http_server(Config config, std::atomic<bool>& running) {
    soap* ctx = soap_new1(SOAP_IO_KEEPALIVE);
    if (!ctx) {
        throw std::runtime_error("failed to allocate gSOAP context");
    }
    ctx->user = &config;
    ctx->fget = health_check;
    ctx->bind_flags = SO_REUSEADDR;

    const SOAP_SOCKET master = soap_bind(ctx, nullptr, config.onvif_port, 16);
    if (!soap_valid_socket(master)) {
        const int err = ctx->errnum;
        soap_free(ctx);
        throw std::runtime_error("failed to bind ONVIF HTTP port: " + std::to_string(err));
    }

    while (running) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(master, &set);
        timeval timeout{};
        timeout.tv_sec = 1;
        const int ready = select(static_cast<int>(master) + 1, &set, nullptr, nullptr, &timeout);
        if (ready <= 0) {
            continue;
        }

        SOAP_SOCKET client = soap_accept(ctx);
        if (!soap_valid_socket(client)) {
            continue;
        }
        soap_serve(ctx);
        soap_destroy(ctx);
        soap_end(ctx);
    }

    soap_done(ctx);
    soap_free(ctx);
}

}  // namespace afterveda_onvif
