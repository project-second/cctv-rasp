#include "utils.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>

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

std::vector<unsigned char> base64_decode(const std::string& value) {
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::array<int, 256> table{};
    table.fill(-1);
    for (int i = 0; i < 64; ++i) {
        table[static_cast<unsigned char>(kAlphabet[i])] = i;
    }

    std::vector<unsigned char> out;
    int bits = 0;
    int accumulator = 0;
    for (const unsigned char c : value) {
        if (c == '=') {
            break;
        }
        if (std::isspace(c)) {
            continue;
        }
        const int decoded = table[c];
        if (decoded < 0) {
            return {};
        }
        accumulator = (accumulator << 6) | decoded;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<unsigned char>((accumulator >> bits) & 0xff));
        }
    }
    return out;
}

std::string base64_encode(const std::vector<unsigned char>& bytes) {
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int bits = 0;
    int accumulator = 0;
    for (const unsigned char byte : bytes) {
        accumulator = (accumulator << 8) | byte;
        bits += 8;
        while (bits >= 6) {
            bits -= 6;
            out += kAlphabet[(accumulator >> bits) & 0x3f];
        }
    }
    if (bits > 0) {
        out += kAlphabet[(accumulator << (6 - bits)) & 0x3f];
    }
    while (out.size() % 4 != 0) {
        out += '=';
    }
    return out;
}

namespace {

uint32_t left_rotate(uint32_t value, int bits) {
    return (value << bits) | (value >> (32 - bits));
}

}  // namespace

std::vector<unsigned char> sha1(const std::vector<unsigned char>& bytes) {
    std::vector<unsigned char> message = bytes;
    const uint64_t bit_len = static_cast<uint64_t>(message.size()) * 8u;
    message.push_back(0x80);
    while ((message.size() % 64) != 56) {
        message.push_back(0);
    }
    for (int i = 7; i >= 0; --i) {
        message.push_back(static_cast<unsigned char>((bit_len >> (i * 8)) & 0xff));
    }

    uint32_t h0 = 0x67452301u;
    uint32_t h1 = 0xefcdab89u;
    uint32_t h2 = 0x98badcfeu;
    uint32_t h3 = 0x10325476u;
    uint32_t h4 = 0xc3d2e1f0u;

    for (std::size_t chunk = 0; chunk < message.size(); chunk += 64) {
        std::array<uint32_t, 80> w{};
        for (int i = 0; i < 16; ++i) {
            const std::size_t j = chunk + static_cast<std::size_t>(i) * 4;
            w[i] = (static_cast<uint32_t>(message[j]) << 24) |
                (static_cast<uint32_t>(message[j + 1]) << 16) |
                (static_cast<uint32_t>(message[j + 2]) << 8) |
                static_cast<uint32_t>(message[j + 3]);
        }
        for (int i = 16; i < 80; ++i) {
            w[i] = left_rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;
        for (int i = 0; i < 80; ++i) {
            uint32_t f = 0;
            uint32_t k = 0;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5a827999u;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ed9eba1u;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8f1bbcdcu;
            } else {
                f = b ^ c ^ d;
                k = 0xca62c1d6u;
            }
            const uint32_t temp = left_rotate(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = left_rotate(b, 30);
            b = a;
            a = temp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    std::vector<unsigned char> digest;
    for (const uint32_t word : {h0, h1, h2, h3, h4}) {
        digest.push_back(static_cast<unsigned char>((word >> 24) & 0xff));
        digest.push_back(static_cast<unsigned char>((word >> 16) & 0xff));
        digest.push_back(static_cast<unsigned char>((word >> 8) & 0xff));
        digest.push_back(static_cast<unsigned char>(word & 0xff));
    }
    return digest;
}

bool utc_timestamp_within(const std::string& value, int tolerance_seconds) {
    std::tm tm{};
    std::istringstream in(value);
    in >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (in.fail()) {
        return false;
    }
#if defined(_GNU_SOURCE) || defined(__linux__)
    const std::time_t parsed = timegm(&tm);
#else
    const std::time_t parsed = std::mktime(&tm);
#endif
    if (parsed == static_cast<std::time_t>(-1)) {
        return false;
    }
    const std::time_t now = std::time(nullptr);
    return std::llabs(static_cast<long long>(now - parsed)) <= tolerance_seconds;
}

}  // namespace afterveda_onvif
