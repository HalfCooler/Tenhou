#pragma once
#include "network_protocol.hpp"
#include <charconv>
#include <stdexcept>
#include <string>

namespace tenhou::protocol_detail {
    inline bool Letter(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }
    inline bool Digit(char c) { return c >= '0' && c <= '9'; }

    [[noreturn]] inline void Fail(const char *message) { throw std::runtime_error(message); }

    inline unsigned Integer(const std::string &value, int base = 10) {
        unsigned result = 0;
        const auto p = std::from_chars(value.data(), value.data() + value.size(), result, base);
        if (value.empty() || p.ec != std::errc() || p.ptr != value.data() + value.size())
            Fail("Invalid protocol number");
        return result;
    }

    inline std::string Attribute(const NetworkMessage &message, const std::string &key) {
        const auto found = message.attributes.find(key);
        return found == message.attributes.end() ? "" : found->second;
    }

    inline std::string Escape(const std::string &value) {
        std::string out;
        for (char c: value) {
            if (c == '&') out += "&amp;";
            else if (c == '"') out += "&quot;";
            else if (c == '<') out += "&lt;";
            else if (c == '>') out += "&gt;";
            else out += c;
        }
        return out;
    }
} // namespace tenhou::protocol_detail
