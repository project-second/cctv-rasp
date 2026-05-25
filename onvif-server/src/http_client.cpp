#include "http_client.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sstream>
#include <stdexcept>

namespace afterveda_onvif {
namespace {

std::pair<std::string, std::string> parse_http_url(const std::string& url) {
    const std::string prefix = "http://";
    if (url.rfind(prefix, 0) != 0) {
        throw std::runtime_error("only http:// rail control URLs are supported");
    }
    const std::string rest = url.substr(prefix.size());
    const std::size_t slash = rest.find('/');
    const std::string authority = slash == std::string::npos ? rest : rest.substr(0, slash);
    const std::string base_path = slash == std::string::npos ? "" : rest.substr(slash);
    return {authority, base_path};
}

}  // namespace

std::optional<std::string> http_request(const std::string& base_url, const std::string& method, const std::string& path) {
    try {
        const auto [authority, base_path] = parse_http_url(base_url);
        std::string host = authority;
        int port = 80;
        const std::size_t colon = authority.rfind(':');
        if (colon != std::string::npos) {
            host = authority.substr(0, colon);
            port = std::stoi(authority.substr(colon + 1));
        }

        const int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            return std::nullopt;
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<uint16_t>(port));
        if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1) {
            close(fd);
            return std::nullopt;
        }

        timeval timeout{};
        timeout.tv_sec = 1;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        if (connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
            close(fd);
            return std::nullopt;
        }

        const std::string full_path = base_path + path;
        std::ostringstream wire;
        wire << method << " " << full_path << " HTTP/1.1\r\n"
             << "Host: " << authority << "\r\n"
             << "Connection: close\r\n"
             << "Content-Length: 0\r\n\r\n";
        const std::string request = wire.str();
        send(fd, request.data(), request.size(), 0);

        std::string response;
        char buffer[2048];
        for (;;) {
            const ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
            if (n <= 0) {
                break;
            }
            response.append(buffer, static_cast<std::size_t>(n));
        }
        close(fd);

        const std::size_t body = response.find("\r\n\r\n");
        if (body == std::string::npos) {
            return std::nullopt;
        }
        return response.substr(body + 4);
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace afterveda_onvif
