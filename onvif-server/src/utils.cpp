#include "utils.h"

#include <cstdint>
#include <ctime>
#include <sstream>

namespace afterveda_onvif {

std::string xml_escape(const std::string& value) {
    std::string escaped;
    for (const char c : value) {
        switch (c) {
        case '&':
            escaped += "&amp;";
            break;
        case '<':
            escaped += "&lt;";
            break;
        case '>':
            escaped += "&gt;";
            break;
        case '"':
            escaped += "&quot;";
            break;
        case '\'':
            escaped += "&apos;";
            break;
        default:
            escaped += c;
            break;
        }
    }
    return escaped;
}

std::string uuid_from_serial(const std::string& serial) {
    uint32_t hash = 2166136261u;
    for (const unsigned char c : serial) {
        hash ^= c;
        hash *= 16777619u;
    }

    std::ostringstream out;
    out << std::hex << "00000000-0000-4000-8000-";
    out.width(8);
    out.fill('0');
    out << hash << "0000";
    return out.str();
}

std::string now_utc() {
    std::time_t raw = std::time(nullptr);
    std::tm tm{};
    gmtime_r(&raw, &tm);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buffer;
}

std::string xaddr_base(const Config& config) {
    return "http://" + config.xaddr_host + ":" + std::to_string(config.onvif_port) + "/onvif";
}

std::string soap_envelope(const std::string& body) {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" "
        "xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" "
        "xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\" "
        "xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\" "
        "xmlns:tt=\"http://www.onvif.org/ver10/schema\">"
        "<s:Body>" + body + "</s:Body></s:Envelope>";
}

std::string soap_fault(const std::string& reason) {
    return soap_envelope(
        "<s:Fault><s:Code><s:Value>s:Sender</s:Value></s:Code>"
        "<s:Reason><s:Text xml:lang=\"en\">" + xml_escape(reason) +
        "</s:Text></s:Reason></s:Fault>");
}

std::string http_wire_response(const HttpResponse& response) {
    std::ostringstream out;
    out << "HTTP/1.1 " << response.status << " " << response.reason << "\r\n"
        << "Content-Type: " << response.content_type << "\r\n"
        << "Content-Length: " << response.body.size() << "\r\n"
        << "Connection: close\r\n\r\n"
        << response.body;
    return out.str();
}

bool contains(const std::string& value, const std::string& needle) {
    return value.find(needle) != std::string::npos;
}

std::string extract_between(const std::string& value, const std::string& start, const std::string& end) {
    const std::size_t begin = value.find(start);
    if (begin == std::string::npos) {
        return {};
    }
    const std::size_t content_begin = begin + start.size();
    const std::size_t content_end = value.find(end, content_begin);
    if (content_end == std::string::npos) {
        return {};
    }
    return value.substr(content_begin, content_end - content_begin);
}

std::string first_nonempty(std::initializer_list<std::string> values) {
    for (const auto& value : values) {
        if (!value.empty()) {
            return value;
        }
    }
    return {};
}

std::string extract_profile_token(const std::string& request) {
    std::string token = first_nonempty({
        extract_between(request, "<trt:ProfileToken>", "</trt:ProfileToken>"),
        extract_between(request, "<ProfileToken>", "</ProfileToken>"),
        extract_between(request, "<trt:ConfigurationToken>", "</trt:ConfigurationToken>"),
        extract_between(request, "<ConfigurationToken>", "</ConfigurationToken>"),
    });
    const std::string prefix = "encoder-";
    if (token.rfind(prefix, 0) == 0) {
        token = token.substr(prefix.size());
    }
    return token;
}

std::string shell_quote(const std::string& value) {
    std::string quoted = "'";
    for (const char c : value) {
        if (c == '\'') {
            quoted += "'\\''";
        } else {
            quoted += c;
        }
    }
    quoted += "'";
    return quoted;
}

}  // namespace afterveda_onvif
