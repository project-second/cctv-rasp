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
bool contains(const std::string& value, const std::string& needle);
std::string extract_between(const std::string& value, const std::string& start, const std::string& end);
std::string first_nonempty(std::initializer_list<std::string> values);
std::vector<unsigned char> base64_decode(const std::string& value);
std::string base64_encode(const std::vector<unsigned char>& bytes);
std::vector<unsigned char> sha1(const std::vector<unsigned char>& bytes);
bool utc_timestamp_within(const std::string& value, int tolerance_seconds);

}  // namespace afterveda_onvif
