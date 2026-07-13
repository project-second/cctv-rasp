#include "discovery_server.h"

#include "utils.h"

#include "soapH.h"
#include "wsddapi.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstring>
#include <stdexcept>
#include <string>

namespace afterveda_onvif {
namespace {

constexpr const char* kDiscoveryAddress = "239.255.255.250";
constexpr int kDiscoveryPort = 3702;
constexpr const char* kDiscoveryTypes = "dn:NetworkVideoTransmitter tds:Device";
constexpr const char* kDiscoveryEndpoint = "soap.udp://239.255.255.250:3702";

Namespace kDiscoveryNamespaces[] = {
    {"SOAP-ENV", "http://www.w3.org/2003/05/soap-envelope", "http://schemas.xmlsoap.org/soap/envelope/", nullptr},
    {"SOAP-ENC", "http://www.w3.org/2003/05/soap-encoding", "http://schemas.xmlsoap.org/soap/encoding/", nullptr},
    {"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", nullptr},
    {"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", nullptr},
    {"wsa", "http://schemas.xmlsoap.org/ws/2004/08/addressing", "http://www.w3.org/2005/08/addressing", nullptr},
    {"wsdd", "http://schemas.xmlsoap.org/ws/2005/04/discovery", nullptr, nullptr},
    {"dn", "http://www.onvif.org/ver10/network/wsdl", nullptr, nullptr},
    {"tds", "http://www.onvif.org/ver10/device/wsdl", nullptr, nullptr},
    {nullptr, nullptr, nullptr, nullptr},
};

std::string discovery_endpoint(const Config& config) {
    return "urn:uuid:" + uuid_from_serial(config.serial);
}

std::string discovery_xaddrs(const Config& config) {
    return xaddr_base(config) + "/device_service";
}

std::string discovery_scopes(const Config& config) {
    return "onvif://www.onvif.org/name/" + config.device_name +
        " onvif://www.onvif.org/hardware/" + config.hardware_id +
        " onvif://www.onvif.org/type/video_encoder"
        " onvif://www.onvif.org/type/NetworkVideoTransmitter"
        " onvif://www.onvif.org/location/country/Korea";
}

std::string discovery_probe_matches_xml(soap* ctx, const Config& config, const char* relates_to) {
    const std::string endpoint = discovery_endpoint(config);
    const std::string xaddrs = discovery_xaddrs(config);
    const std::string scopes = discovery_scopes(config);
    const char* message_id = soap_wsa_rand_uuid(ctx);
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" "
        "xmlns:a=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
        "xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" "
        "xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" "
        "xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\">"
        "<s:Header>"
        "<a:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches</a:Action>"
        "<a:MessageID>" + std::string(message_id ? message_id : "uuid:afterveda-probematch") + "</a:MessageID>"
        "<a:RelatesTo>" + xml_escape(relates_to ? relates_to : "") + "</a:RelatesTo>"
        "<a:To>http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</a:To>"
        "</s:Header>"
        "<s:Body>"
        "<d:ProbeMatches>"
        "<d:ProbeMatch>"
        "<a:EndpointReference><a:Address>" + xml_escape(endpoint) + "</a:Address></a:EndpointReference>"
        "<d:Types>" + std::string(kDiscoveryTypes) + "</d:Types>"
        "<d:Scopes>" + xml_escape(scopes) + "</d:Scopes>"
        "<d:XAddrs>" + xml_escape(xaddrs) + "</d:XAddrs>"
        "<d:MetadataVersion>1</d:MetadataVersion>"
        "</d:ProbeMatch>"
        "</d:ProbeMatches>"
        "</s:Body>"
        "</s:Envelope>";
}

void send_discovery_udp_response(soap* ctx, const std::string& response) {
    const SOAP_SOCKET socket = soap_valid_socket(ctx->master) ? ctx->master : ctx->socket;
    if (!soap_valid_socket(socket) || ctx->peerlen == 0) {
        return;
    }
    sendto(socket, response.data(), response.size(), 0, &ctx->peer.addr, static_cast<socklen_t>(ctx->peerlen));
}

soap* new_discovery_announce_context() {
    soap* ctx = soap_new1(SOAP_IO_UDP);
    if (!ctx) {
        return nullptr;
    }
    ctx->connect_flags = SO_BROADCAST;
    ctx->ipv4_multicast_ttl = 1;
    soap_set_namespaces(ctx, kDiscoveryNamespaces);
    return ctx;
}

void free_discovery_announce_context(soap* ctx) {
    if (!ctx) {
        return;
    }
    soap_destroy(ctx);
    soap_end(ctx);
    soap_done(ctx);
    soap_free(ctx);
}

void announce_discovery_hello(const Config& config) {
    soap* ctx = new_discovery_announce_context();
    if (!ctx) {
        return;
    }
    const std::string endpoint = discovery_endpoint(config);
    const std::string xaddrs = discovery_xaddrs(config);
    const std::string scopes = discovery_scopes(config);
    soap_wsdd_Hello(ctx, SOAP_WSDD_ADHOC, kDiscoveryEndpoint, soap_wsa_rand_uuid(ctx), nullptr,
        endpoint.c_str(), kDiscoveryTypes, scopes.c_str(), nullptr, xaddrs.c_str(), 1);
    free_discovery_announce_context(ctx);
}

void announce_discovery_bye(const Config& config) {
    soap* ctx = new_discovery_announce_context();
    if (!ctx) {
        return;
    }
    const std::string endpoint = discovery_endpoint(config);
    const std::string xaddrs = discovery_xaddrs(config);
    const std::string scopes = discovery_scopes(config);
    soap_wsdd_Bye(ctx, SOAP_WSDD_ADHOC, kDiscoveryEndpoint, soap_wsa_rand_uuid(ctx),
        endpoint.c_str(), kDiscoveryTypes, scopes.c_str(), nullptr, xaddrs.c_str(), 1);
    free_discovery_announce_context(ctx);
}

}  // namespace

void discovery_server(Config config, std::atomic<bool>& running) {
    soap* ctx = soap_new1(SOAP_IO_UDP);
    if (!ctx) {
        throw std::runtime_error("failed to allocate WS-Discovery gSOAP context");
    }
    ctx->user = &config;
    ctx->bind_flags = SO_REUSEADDR;
    ctx->connect_flags = SO_BROADCAST;
    ctx->ipv4_multicast_ttl = 1;
    soap_set_namespaces(ctx, kDiscoveryNamespaces);

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

    announce_discovery_hello(config);

    while (running) {
        soap_wsdd_listen(ctx, -1000000);
        soap_destroy(ctx);
        soap_end(ctx);
    }

    announce_discovery_bye(config);

    soap_done(ctx);
    soap_free(ctx);
}

}  // namespace afterveda_onvif

soap_wsdd_mode wsdd_event_Probe(soap* ctx, const char* message_id, const char*, const char*, const char*, const char*, wsdd__ProbeMatchesType*) {
    const auto& config = *static_cast<afterveda_onvif::Config*>(ctx->user);
    afterveda_onvif::send_discovery_udp_response(ctx, afterveda_onvif::discovery_probe_matches_xml(ctx, config, message_id));
    return SOAP_WSDD_ADHOC;
}

soap_wsdd_mode wsdd_event_Resolve(soap* ctx, const char*, const char*, const char*, wsdd__ResolveMatchType* match) {
    const auto& config = *static_cast<afterveda_onvif::Config*>(ctx->user);
    const std::string endpoint = afterveda_onvif::discovery_endpoint(config);
    const std::string xaddrs = afterveda_onvif::discovery_xaddrs(config);
    const std::string scopes = afterveda_onvif::discovery_scopes(config);
    soap_default_wsdd__ResolveMatchType(ctx, match);
    match->wsa__EndpointReference.Address = soap_strdup(ctx, endpoint.c_str());
    match->Types = soap_strdup(ctx, afterveda_onvif::kDiscoveryTypes);
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
