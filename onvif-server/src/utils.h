#pragma once

#include "config.h"
#include "types.h"

#include <initializer_list>
#include <string>

namespace afterveda_onvif {

std::string xml_escape(const std::string& value);
std::string uuid_from_serial(const std::string& serial);
std::string now_utc();
std::string xaddr_base(const Config& config);
std::string soap_envelope(const std::string& body);
std::string soap_fault(const std::string& reason);
std::string http_wire_response(const HttpResponse& response);
bool contains(const std::string& value, const std::string& needle);
std::string extract_between(const std::string& value, const std::string& start, const std::string& end);
std::string first_nonempty(std::initializer_list<std::string> values);
std::string extract_profile_token(const std::string& request);
std::string shell_quote(const std::string& value);

}  // namespace afterveda_onvif
