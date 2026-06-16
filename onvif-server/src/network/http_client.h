#pragma once

#include <optional>
#include <string>

namespace afterveda_onvif {

std::optional<std::string> http_request(const std::string& base_url, const std::string& method, const std::string& path);

}  // namespace afterveda_onvif
