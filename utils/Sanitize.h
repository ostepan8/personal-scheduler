#pragma once
#include <string>
#include <algorithm>
#include <iostream>

inline std::string sanitize(const std::string &in, size_t maxLen = 256)
{
    std::cout << "Sanitizing string of length " << in.size() << " with max length " << maxLen << std::endl;

    std::string out;
    out.reserve(std::min(maxLen, in.size()));

    for (char c : in)
    {
        if (std::iscntrl(static_cast<unsigned char>(c)))
        {
            std::cout << "Skipping control character: " << static_cast<int>(c) << std::endl;
            continue;
        }
        out.push_back(c);
        if (out.size() >= maxLen)
        {
            std::cout << "Reached max length of " << maxLen << ", truncating" << std::endl;
            break;
        }
    }

    std::cout << "Sanitized result length: " << out.size() << std::endl;
    return out;
}
