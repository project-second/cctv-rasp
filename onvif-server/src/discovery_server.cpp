#include "discovery_server.h"

#include "utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdexcept>
#include <string>

namespace afterveda_onvif {
namespace {

constexpr const char* kDiscoveryAddress = "239.255.255.250";
constexpr int kDiscoveryPort = 3702;

std::string discovery_response(const Config& config, const std::string& message_id) {
    const std::string endpoint = "urn:uuid:" + uuid_from_serial(config.serial);
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<e:Envelope xmlns:e=\"http://www.w3.org/2003/05/soap-envelope\" "
        "xmlns:w=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
        "xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" "
        "xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\">"
        "<e:Header>"
        "<w:MessageID>urn:uuid:" + uuid_from_serial(config.serial + "-response") + "</w:MessageID>"
        "<w:RelatesTo>" + xml_escape(message_id) + "</w:RelatesTo>"
        "<w:To>http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</w:To>"
        "<w:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches</w:Action>"
        "</e:Header>"
        "<e:Body><d:ProbeMatches><d:ProbeMatch>"
        "<w:EndpointReference><w:Address>" + endpoint + "</w:Address></w:EndpointReference>"
        "<d:Types>dn:NetworkVideoTransmitter</d:Types>"
        "<d:Scopes>onvif://www.onvif.org/name/" + xml_escape(config.device_name) + " onvif://www.onvif.org/hardware/" + xml_escape(config.hardware_id) + "</d:Scopes>"
        "<d:XAddrs>" + xml_escape(xaddr_base(config) + "/device_service") + "</d:XAddrs>"
        "<d:MetadataVersion>1</d:MetadataVersion>"
        "</d:ProbeMatch></d:ProbeMatches></e:Body></e:Envelope>";
}

}  // namespace

void discovery_server(Config config, std::atomic<bool>& running) {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        throw std::runtime_error("failed to open WS-Discovery socket");
    }

    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(kDiscoveryPort);
    if (bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        close(fd);
        throw std::runtime_error("failed to bind WS-Discovery UDP port 3702");
    }

    ip_mreq membership{};
    membership.imr_multiaddr.s_addr = inet_addr(kDiscoveryAddress);
    membership.imr_interface.s_addr = INADDR_ANY;
    setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &membership, sizeof(membership));

    while (running) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);
        timeval timeout{};
        timeout.tv_sec = 1;
        const int ready = select(fd + 1, &set, nullptr, nullptr, &timeout);
        if (ready <= 0) {
            continue;
        }

        sockaddr_in client{};
        socklen_t client_len = sizeof(client);
        char buffer[8192];
        const ssize_t n = recvfrom(fd, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr*>(&client), &client_len);
        if (n <= 0) {
            continue;
        }
        buffer[n] = '\0';
        const std::string request(buffer);
        if (!contains(request, "Probe") && !contains(request, "Resolve")) {
            continue;
        }
        const std::string message_id = first_nonempty({
            extract_between(request, "<a:MessageID>", "</a:MessageID>"),
            extract_between(request, "<w:MessageID>", "</w:MessageID>"),
        });
        const std::string response = discovery_response(config, message_id);
        sendto(fd, response.data(), response.size(), 0, reinterpret_cast<sockaddr*>(&client), client_len);
    }

    close(fd);
}

}  // namespace afterveda_onvif
