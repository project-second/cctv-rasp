#include "http_server.h"

#include "soap_services.h"
#include "utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdexcept>
#include <string>

namespace afterveda_onvif {

void http_server(Config config, std::atomic<bool>& running) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("failed to open ONVIF HTTP socket");
    }

    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(config.onvif_port));
    if (bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        close(fd);
        throw std::runtime_error("failed to bind ONVIF HTTP port");
    }
    if (listen(fd, 16) < 0) {
        close(fd);
        throw std::runtime_error("failed to listen on ONVIF HTTP port");
    }

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

        const int client = accept(fd, nullptr, nullptr);
        if (client < 0) {
            continue;
        }

        std::string request;
        char buffer[8192];
        const ssize_t n = recv(client, buffer, sizeof(buffer), 0);
        if (n > 0) {
            request.assign(buffer, static_cast<std::size_t>(n));
        }

        HttpResponse response;
        if (request.empty()) {
            response = {400, "Bad Request", "application/json", "{\"error\":\"bad request\"}\n"};
        } else if (request.rfind("GET / ", 0) == 0 || request.rfind("GET /onvif", 0) == 0) {
            response = {200, "OK", "application/json", "{\"status\":\"ok\",\"service\":\"afterveda-onvif\"}\n"};
        } else {
            response = handle_soap(config, request);
        }
        const std::string wire = http_wire_response(response);
        send(client, wire.data(), wire.size(), 0);
        close(client);
    }

    close(fd);
}

}  // namespace afterveda_onvif
