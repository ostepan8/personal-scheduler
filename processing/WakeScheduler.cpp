#include "WakeScheduler.h"
#include "../utils/BuiltinActions.h"
#include "../utils/TimeUtils.h"
#include "../scheduler/ScheduledTask.h"
#include "nlohmann/json.hpp"
#include <ctime>
#include <algorithm>
#include <sstream>

using namespace std;
using namespace std::chrono;

WakeScheduler::WakeScheduler(Model &model, EventLoop &loop, SettingsStore &settings)
    : model_(model), loop_(loop), settings_(settings) {}

system_clock::time_point WakeScheduler::localMidnight(system_clock::time_point tp) const {
    time_t t = system_clock::to_time_t(tp);
    std::tm tm{};
#if defined(_MSC_VER)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
    return system_clock::from_time_t(mktime(&tm));
}

system_clock::time_point WakeScheduler::parseLocalTimeHM(system_clock::time_point day, const std::string &hm) const {
    int hh = 2, mm = 0;
    if (!hm.empty()) {
        std::sscanf(hm.c_str(), "%d:%d", &hh, &mm);
    }
    time_t t = system_clock::to_time_t(day);
    std::tm tm{};
#if defined(_MSC_VER)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    tm.tm_hour = hh; tm.tm_min = mm; tm.tm_sec = 0;
    return system_clock::from_time_t(mktime(&tm));
}

system_clock::time_point WakeScheduler::nextLocalMidnight(system_clock::time_point now) const {
    // Compute local time components and roll day by one
    time_t t = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_MSC_VER)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    tm.tm_mday += 1;
    tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
    return system_clock::from_time_t(mktime(&tm));
}

system_clock::time_point WakeScheduler::computeWakeTime(system_clock::time_point day,
                                                        std::string &reason,
                                                        std::vector<Event> &firstEvents) const {
    // Load config
    auto baselineStr = settings_.getString("wake.baseline_time").value_or("02:00");
    int lead = settings_.getInt("wake.lead_minutes").value_or(45);
    bool onlyWhenEvents = settings_.getBool("wake.only_when_events").value_or(false);
    bool skipWeekends = settings_.getBool("wake.skip_weekends").value_or(false);

    auto base = parseLocalTimeHM(day, baselineStr);
    // Collect first few events on day
    auto events = model_.getEventsOnDay(day);
    std::sort(events.begin(), events.end(), [](const Event &a, const Event &b){ return a.getTime() < b.getTime(); });
    firstEvents.clear();
    for (size_t i = 0; i < events.size() && i < 3; ++i) firstEvents.push_back(events[i]);

    // Weekend skip logic
    if (events.empty()) {
        if (onlyWhenEvents) {
            reason = "no-events-skip";
            return system_clock::time_point::min();
        }
        if (skipWeekends) {
            time_t t = system_clock::to_time_t(day);
            std::tm tm{};
#if defined(_MSC_VER)
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif
        if (tm.tm_wday == 0 || tm.tm_wday == 6) { // Sun/Sat
            reason = "weekend-skip";
            return system_clock::time_point::min();
        }
        }
        reason = "baseline";
        return base;
    }

    auto earliest = events.front().getTime();
    auto candidate = earliest - minutes(lead);
    if (onlyWhenEvents) {
        reason = "earliest-minus-lead";
        return candidate;
    }
    if (candidate < base) {
        reason = "earliest-minus-lead";
        return candidate;
    }
    reason = "baseline";
    return base;
}

void WakeScheduler::scheduleToday() {
    if (!settings_.getBool("wake.enabled").value_or(true)) return;
    auto now = system_clock::now();
    auto day = localMidnight(now);
    std::string reason;
    std::vector<Event> first;
    auto wakeTime = computeWakeTime(day, reason, first);
    if (wakeTime == system_clock::time_point::min()) return; // skip
    if (wakeTime <= now) return; // already passed

    // Build action that posts to external server with context
    std::string url = settings_.getString("wake.server_url").value_or("");
    auto action = [url, day, wakeTime, reason, first]() {
        if (url.empty()) {
            std::cout << "[wake] No server URL configured; skipping call\n";
            return;
        }
        nlohmann::json payload;
        payload["date"] = TimeUtils::formatTimePoint(day);
        payload["wake_time"] = TimeUtils::formatTimePoint(wakeTime);
        payload["reason"] = reason;
        nlohmann::json earliest = nullptr;
        if (!first.empty()) {
            earliest = {
                {"id", first[0].getId()},
                {"title", first[0].getTitle()},
                {"description", first[0].getDescription()},
                {"time", TimeUtils::formatTimePoint(first[0].getTime())},
                {"duration", std::chrono::duration_cast<std::chrono::seconds>(first[0].getDuration()).count()}
            };
        }
        payload["earliest_event"] = earliest;
        nlohmann::json brief = nlohmann::json::array();
        for (const auto &e : first) {
            brief.push_back({
                {"id", e.getId()},
                {"title", e.getTitle()},
                {"time", TimeUtils::formatTimePoint(e.getTime())}
            });
        }
        payload["first_events"] = brief;
        BuiltinActions::httpPostJson(url, payload.dump());
    };

    // Build task with deterministic ID
    char datebuf[11]; // YYYY-MM-DD + null
    auto ds = TimeUtils::formatTimePoint(day);
    std::snprintf(datebuf, sizeof(datebuf), "%.*s", 10, ds.c_str());
    std::string id = std::string("wake:") + datebuf;
    std::string title = std::string("Wake for ") + datebuf;
    auto task = std::make_shared<ScheduledTask>(id, "wake task", title, wakeTime, seconds(0),
                                                std::vector<system_clock::time_point>{}, []{}, action);
    task->setCategory("internal");
    loop_.addTask(task);
}

void WakeScheduler::scheduleDailyMaintenance() {
    // Compute next local midnight and schedule a task to reschedule today's wake
    auto now = system_clock::now();
    auto nextMid = nextLocalMidnight(now);
    auto task = std::make_shared<ScheduledTask>(
        "wake:maintenance", "wake maintenance", "Wake Maintenance",
        nextMid, seconds(0), std::vector<system_clock::time_point>{}, []{},
        [this]() { const_cast<WakeScheduler*>(this)->scheduleToday(); const_cast<WakeScheduler*>(this)->scheduleDailyMaintenance(); }
    );
    task->setCategory("internal");
    loop_.addTask(task);
}

void WakeScheduler::scheduleForDate(system_clock::time_point day) {
    if (!settings_.getBool("wake.enabled").value_or(true)) return;
    std::string reason;
    std::vector<Event> first;
    auto wakeTime = computeWakeTime(day, reason, first);
    auto now = system_clock::now();
    if (wakeTime == system_clock::time_point::min() || wakeTime <= now) return;

    // Construct deterministic ID based on date
    char datebuf[11];
    auto ds = TimeUtils::formatTimePoint(day);
    std::snprintf(datebuf, sizeof(datebuf), "%.*s", 10, ds.c_str());
    std::string id = std::string("wake:") + datebuf;
    std::string title = std::string("Wake for ") + datebuf;

    std::string url = settings_.getString("wake.server_url").value_or("");
    auto action = [url, day, wakeTime, reason, first]() {
        if (url.empty()) return;
        nlohmann::json payload;
        payload["date"] = TimeUtils::formatTimePoint(day);
        payload["wake_time"] = TimeUtils::formatTimePoint(wakeTime);
        payload["reason"] = reason;
        nlohmann::json earliest = nullptr;
        if (!first.empty()) {
            earliest = {
                {"id", first[0].getId()},
                {"title", first[0].getTitle()},
                {"description", first[0].getDescription()},
                {"time", TimeUtils::formatTimePoint(first[0].getTime())},
                {"duration", std::chrono::duration_cast<std::chrono::seconds>(first[0].getDuration()).count()}
            };
        }
        payload["earliest_event"] = earliest;
        nlohmann::json brief = nlohmann::json::array();
        for (const auto &e : first) {
            brief.push_back({
                {"id", e.getId()},
                {"title", e.getTitle()},
                {"time", TimeUtils::formatTimePoint(e.getTime())}
            });
        }
        payload["first_events"] = brief;
        BuiltinActions::httpPostJson(url, payload.dump());
    };

    auto task = std::make_shared<ScheduledTask>(id, "wake task", title, wakeTime, seconds(0),
                                                std::vector<system_clock::time_point>{}, []{}, action);
    task->setCategory("internal");
    loop_.addTask(task);
}

std::chrono::system_clock::time_point WakeScheduler::previewForDate(system_clock::time_point day,
                                                                    std::string &reason,
                                                                    std::vector<Event> &firstEvents) const {
    return computeWakeTime(day, reason, firstEvents);
}
