#pragma once

#include <optional>
#include <string>

namespace afterveda_onvif {

struct RailHttpResponse {
    int status = 0;
    std::string body;
};

std::optional<RailHttpResponse> http_request_with_status(
    const std::string& base_url,
    const std::string& method,
    const std::string& path,
    const std::string& body = "");
std::optional<std::string> http_request(const std::string& base_url, const std::string& method, const std::string& path);

}  // namespace afterveda_onvif
