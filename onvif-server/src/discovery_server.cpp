#include "discovery_server.h"

#include "utils.h"

#include "soapH.h"
#include "wsddapi.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <stdexcept>
#include <string>

namespace afterveda_onvif {
namespace {

constexpr const char* kDiscoveryAddress = "239.255.255.250";
constexpr int kDiscoveryPort = 3702;

}  // namespace

void discovery_server(Config config, std::atomic<bool>& running) {
    soap* ctx = soap_new1(SOAP_IO_UDP);
    if (!ctx) {
        throw std::runtime_error("failed to allocate WS-Discovery gSOAP context");
    }
    ctx->user = &config;
    ctx->bind_flags = SO_REUSEADDR;

    const SOAP_SOCKET master = soap_bind(ctx, nullptr, kDiscoveryPort, 100);
    if (!soap_valid_socket(master)) {
        const int err = ctx->errnum;
        soap_free(ctx);
        throw std::runtime_error("failed to bind WS-Discovery UDP port 3702: " + std::to_string(err));
    }

    ip_mreq membership{};
    membership.imr_multiaddr.s_addr = inet_addr(kDiscoveryAddress);
    membership.imr_interface.s_addr = INADDR_ANY;
    setsockopt(master, IPPROTO_IP, IP_ADD_MEMBERSHIP, &membership, sizeof(membership));

    while (running) {
        soap_wsdd_listen(ctx, -1000000);
        soap_destroy(ctx);
        soap_end(ctx);
    }

    soap_done(ctx);
    soap_free(ctx);
}

}  // namespace afterveda_onvif

soap_wsdd_mode wsdd_event_Probe(soap* ctx, const char*, const char*, const char*, const char*, const char*, wsdd__ProbeMatchesType* matches) {
    const auto& config = *static_cast<afterveda_onvif::Config*>(ctx->user);
    const std::string endpoint = "urn:uuid:" + afterveda_onvif::uuid_from_serial(config.serial);
    const std::string xaddrs = afterveda_onvif::xaddr_base(config) + "/device_service";
    const std::string scopes = "onvif://www.onvif.org/name/" + config.device_name +
        " onvif://www.onvif.org/hardware/" + config.hardware_id +
        " onvif://www.onvif.org/Profile/T";
    soap_wsdd_add_ProbeMatch(ctx, matches, endpoint.c_str(), "dn:NetworkVideoTransmitter", scopes.c_str(), nullptr, xaddrs.c_str(), 1);
    return SOAP_WSDD_MANAGED;
}

soap_wsdd_mode wsdd_event_Resolve(soap* ctx, const char*, const char*, const char*, wsdd__ResolveMatchType* match) {
    const auto& config = *static_cast<afterveda_onvif::Config*>(ctx->user);
    const std::string endpoint = "urn:uuid:" + afterveda_onvif::uuid_from_serial(config.serial);
    const std::string xaddrs = afterveda_onvif::xaddr_base(config) + "/device_service";
    const std::string scopes = "onvif://www.onvif.org/name/" + config.device_name +
        " onvif://www.onvif.org/hardware/" + config.hardware_id +
        " onvif://www.onvif.org/Profile/T";
    soap_default_wsdd__ResolveMatchType(ctx, match);
    match->wsa__EndpointReference.Address = soap_strdup(ctx, endpoint.c_str());
    match->Types = soap_strdup(ctx, "dn:NetworkVideoTransmitter");
    match->Scopes = soap_new_wsdd__ScopesType(ctx);
    match->Scopes->__item = soap_strdup(ctx, scopes.c_str());
    match->XAddrs = soap_strdup(ctx, xaddrs.c_str());
    match->MetadataVersion = 1;
    return SOAP_WSDD_MANAGED;
}

void wsdd_event_Hello(soap*, unsigned int, const char*, unsigned int, const char*, const char*, const char*, const char*, const char*, const char*, const char*, unsigned int) {}
void wsdd_event_Bye(soap*, unsigned int, const char*, unsigned int, const char*, const char*, const char*, const char*, const char*, const char*, const char*, unsigned int*) {}
void wsdd_event_ProbeMatches(soap*, unsigned int, const char*, unsigned int, const char*, const char*, wsdd__ProbeMatchesType*) {}
void wsdd_event_ResolveMatches(soap*, unsigned int, const char*, unsigned int, const char*, const char*, wsdd__ResolveMatchType*) {}
