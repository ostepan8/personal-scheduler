#pragma once
#include <string>
#include <algorithm>

inline std::string sanitize(const std::string &in, size_t maxLen = 256) {
    std::string out;
    out.reserve(std::min(maxLen, in.size()));
    for (char c : in) {
        if (std::iscntrl(static_cast<unsigned char>(c))) continue;
        out.push_back(c);
        if (out.size() >= maxLen) break;
    }
    return out;
}
