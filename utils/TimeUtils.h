#pragma once
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace TimeUtils {
inline std::string formatTimePoint(const std::chrono::system_clock::time_point &tp) {
    using namespace std::chrono;
    time_t t_c = system_clock::to_time_t(tp);
    std::tm tm_buf;
#if defined(_MSC_VER)
    localtime_s(&tm_buf, &t_c);
#else
    localtime_r(&t_c, &tm_buf);
#endif
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm_buf);
    return std::string(buf);
}

inline std::chrono::system_clock::time_point parseTimePoint(const std::string &timestamp) {
    using namespace std::chrono;
    std::tm tm_buf{};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm_buf, "%Y-%m-%d %H:%M");
    if (ss.fail())
        throw std::runtime_error("Invalid timestamp format");
    tm_buf.tm_isdst = -1;
    time_t time_c = std::mktime(&tm_buf);
    return system_clock::from_time_t(time_c);
}

inline std::chrono::system_clock::time_point parseDate(const std::string &dateStr) {
    return parseTimePoint(dateStr + " 00:00");
}

inline std::chrono::system_clock::time_point parseMonth(const std::string &monthStr) {
    return parseTimePoint(monthStr + "-01 00:00");
}
}
