#include "http_server.h"

#include "soapH.h"

#include <stdsoap2.h>

#include <sys/select.h>

#include <stdexcept>
#include <string>

extern "C" int soap_serve(struct soap*);

namespace afterveda_onvif {
namespace {

int health_check(struct soap* ctx) {
    static constexpr const char* kBody = "{\"status\":\"ok\",\"service\":\"afterveda-onvif\"}\n";
    if (soap_response(ctx, SOAP_FILE) != SOAP_OK) {
        return ctx->error;
    }
    soap_send(ctx, "Content-Type: application/json\r\n");
    soap_send(ctx, "Content-Length: ");
    soap_send(ctx, std::to_string(std::char_traits<char>::length(kBody)).c_str());
    soap_send(ctx, "\r\n\r\n");
    soap_send(ctx, kBody);
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
