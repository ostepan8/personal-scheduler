#pragma once
#include <string>
#include <chrono>
#include <format>
#include <sstream>
#include <cstdlib>

namespace TimeUtils {
inline std::string formatTimePoint(const std::chrono::system_clock::time_point &tp) {
    using namespace std::chrono;
    const char* env = std::getenv("TZ");
    auto zone = env ? locate_zone(env) : current_zone();
    zoned_time zt{zone, tp};
    return std::format("{:%Y-%m-%d %H:%M}", zt);
}

inline std::chrono::system_clock::time_point parseTimePoint(const std::string &timestamp) {
    using namespace std::chrono;
    std::tm tm_buf{};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm_buf, "%Y-%m-%d %H:%M");
    if (ss.fail())
        throw std::runtime_error("Invalid timestamp format");
    year_month_day ymd{year{tm_buf.tm_year + 1900}, month{static_cast<unsigned>(tm_buf.tm_mon + 1)}, day{static_cast<unsigned>(tm_buf.tm_mday)}};
    auto local = local_days{ymd} + hours{tm_buf.tm_hour} + minutes{tm_buf.tm_min} + seconds{tm_buf.tm_sec};
    const char* env = std::getenv("TZ");
    auto zone = env ? locate_zone(env) : current_zone();
    return zone->to_sys(local);
}

inline std::chrono::system_clock::time_point parseDate(const std::string &dateStr) {
    return parseTimePoint(dateStr + " 00:00");
}

inline std::chrono::system_clock::time_point parseMonth(const std::string &monthStr) {
    return parseTimePoint(monthStr + "-01 00:00");
}
}
