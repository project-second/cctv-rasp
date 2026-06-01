#pragma once

#include "config.h"
#include "types.h"

#include <initializer_list>
#include <string>
#include <vector>

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
std::string local_xml_name(const std::string& name);
std::string soap_body(const std::string& request);
std::string soap_action(const std::string& request);
std::string xml_text_for_local_name(const std::string& xml, const std::string& local_name);
std::string xml_attribute_for_local_name(const std::string& xml, const std::string& local_name, const std::string& attribute);
std::vector<unsigned char> base64_decode(const std::string& value);
std::string base64_encode(const std::vector<unsigned char>& bytes);
std::vector<unsigned char> sha1(const std::vector<unsigned char>& bytes);
bool utc_timestamp_within(const std::string& value, int tolerance_seconds);

}  // namespace afterveda_onvif
