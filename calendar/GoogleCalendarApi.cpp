#include "GoogleCalendarApi.h"
#include "../utils/TimeUtils.h"
#include "../model/RecurringEvent.h"
#include "../model/recurrence/DailyRecurrence.h"
#include "../model/recurrence/WeeklyRecurrence.h"
#include "../model/recurrence/MonthlyRecurrence.h"
#include "../model/recurrence/YearlyRecurrence.h"
#include "../utils/WeekDay.h"
#include <cstdlib>
#include <string>
#include <sstream>

GoogleCalendarApi::GoogleCalendarApi(std::string creds, std::string calendarId)
    : credentials_(std::move(creds)), calendarId_(std::move(calendarId)) {}

static void setEnv(const char* key, const std::string& value) {
#ifdef _WIN32
    _putenv_s(key, value.c_str());
#else
    setenv(key, value.c_str(), 1);
#endif
}

static void unsetEnv(const char* key) {
#ifdef _WIN32
    _putenv_s(key, "");
#else
    unsetenv(key);
#endif
}

static std::string formatUntil(std::chrono::system_clock::time_point tp) {
    using namespace std::chrono;
    std::time_t t = system_clock::to_time_t(tp);
    std::tm tm_buf;
#if defined(_MSC_VER)
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    char buf[32];
    strftime(buf, sizeof(buf), "%Y%m%dT%H%M%SZ", &tm_buf);
    return std::string(buf);
}

static std::string weekdayCode(Weekday w) {
    switch (w) {
        case Weekday::Sunday: return "SU";
        case Weekday::Monday: return "MO";
        case Weekday::Tuesday: return "TU";
        case Weekday::Wednesday: return "WE";
        case Weekday::Thursday: return "TH";
        case Weekday::Friday: return "FR";
        case Weekday::Saturday: return "SA";
    }
    return "";
}

static std::string rruleFromPattern(const std::shared_ptr<RecurrencePattern>& p) {
    if (!p) return "";
    std::ostringstream r;
    auto type = p->type();
    if (type == "daily") {
        auto* d = dynamic_cast<DailyRecurrence*>(p.get());
        r << "RRULE:FREQ=DAILY";
        if (d->getInterval() > 1) r << ";INTERVAL=" << d->getInterval();
        if (d->getMaxOccurrences() != -1) r << ";COUNT=" << d->getMaxOccurrences();
        if (d->getEndDate() != std::chrono::system_clock::time_point::max())
            r << ";UNTIL=" << formatUntil(d->getEndDate());
    } else if (type == "weekly") {
        auto* w = dynamic_cast<WeeklyRecurrence*>(p.get());
        r << "RRULE:FREQ=WEEKLY";
        if (w->getInterval() > 1) r << ";INTERVAL=" << w->getInterval();
        const auto& days = w->getDaysOfWeek();
        if (!days.empty()) {
            r << ";BYDAY=";
            for (size_t i = 0; i < days.size(); ++i) {
                r << weekdayCode(days[i]);
                if (i + 1 < days.size()) r << ",";
            }
        }
        if (w->getMaxOccurrences() != -1) r << ";COUNT=" << w->getMaxOccurrences();
        if (w->getEndDate() != std::chrono::system_clock::time_point::max())
            r << ";UNTIL=" << formatUntil(w->getEndDate());
    } else if (type == "monthly") {
        auto* m = dynamic_cast<MonthlyRecurrence*>(p.get());
        r << "RRULE:FREQ=MONTHLY";
        if (m->getInterval() > 1) r << ";INTERVAL=" << m->getInterval();
        if (m->getMaxOccurrences() != -1) r << ";COUNT=" << m->getMaxOccurrences();
        if (m->getEndDate() != std::chrono::system_clock::time_point::max())
            r << ";UNTIL=" << formatUntil(m->getEndDate());
    } else if (type == "yearly") {
        auto* y = dynamic_cast<YearlyRecurrence*>(p.get());
        r << "RRULE:FREQ=YEARLY";
        if (y->getInterval() > 1) r << ";INTERVAL=" << y->getInterval();
        if (y->getMaxOccurrences() != -1) r << ";COUNT=" << y->getMaxOccurrences();
        if (y->getEndDate() != std::chrono::system_clock::time_point::max())
            r << ";UNTIL=" << formatUntil(y->getEndDate());
    }
    return r.str();
}

void GoogleCalendarApi::addEvent(const Event &e) {
    setEnv("GCAL_ACTION", "add");
    setEnv("GCAL_CREDS", credentials_);
    setEnv("GCAL_CALENDAR_ID", calendarId_);
    setEnv("GCAL_TITLE", e.getTitle());
    setEnv("GCAL_DESC", e.getDescription());
    setEnv("GCAL_EVENT_ID", e.getId());
    auto start = TimeUtils::formatRFC3339UTC(e.getTime());
    auto end = TimeUtils::formatRFC3339UTC(e.getTime() + e.getDuration());
    setEnv("GCAL_START", start);
    setEnv("GCAL_END", end);
    setEnv("GCAL_TZ", "UTC");

    // Handle recurrence if this is a RecurringEvent
    if (auto re = dynamic_cast<const RecurringEvent*>(&e)) {
        auto rrule = rruleFromPattern(re->getRecurrencePattern());
        if (!rrule.empty()) {
            setEnv("GCAL_RECURRENCE", rrule);
        }
    }
    std::system("python3 -m calendar_integration.gcal_service");
    unsetEnv("GCAL_ACTION");
    unsetEnv("GCAL_CREDS");
    unsetEnv("GCAL_CALENDAR_ID");
    unsetEnv("GCAL_TITLE");
    unsetEnv("GCAL_DESC");
    unsetEnv("GCAL_EVENT_ID");
    unsetEnv("GCAL_START");
    unsetEnv("GCAL_END");
    unsetEnv("GCAL_TZ");
    unsetEnv("GCAL_RECURRENCE");
}

void GoogleCalendarApi::deleteEvent(const Event &e) {
    setEnv("GCAL_ACTION", "delete");
    setEnv("GCAL_CREDS", credentials_);
    setEnv("GCAL_CALENDAR_ID", calendarId_);
    setEnv("GCAL_EVENT_ID", e.getId());
    std::system("python3 -m calendar_integration.gcal_service");
    unsetEnv("GCAL_ACTION");
    unsetEnv("GCAL_CREDS");
    unsetEnv("GCAL_CALENDAR_ID");
    unsetEnv("GCAL_EVENT_ID");
}

void GoogleCalendarApi::updateEvent(const Event &oldEvent, const Event &newEvent) {
    setEnv("GCAL_ACTION", "update");
    setEnv("GCAL_CREDS", credentials_);
    setEnv("GCAL_CALENDAR_ID", calendarId_);
    setEnv("GCAL_TITLE", newEvent.getTitle());
    setEnv("GCAL_DESC", newEvent.getDescription());
    setEnv("GCAL_EVENT_ID", oldEvent.getId());
    auto start = TimeUtils::formatRFC3339UTC(newEvent.getTime());
    auto end = TimeUtils::formatRFC3339UTC(newEvent.getTime() + newEvent.getDuration());
    setEnv("GCAL_START", start);
    setEnv("GCAL_END", end);
    setEnv("GCAL_TZ", "UTC");
    if (auto re = dynamic_cast<const RecurringEvent*>(&newEvent)) {
        auto rrule = rruleFromPattern(re->getRecurrencePattern());
        if (!rrule.empty()) setEnv("GCAL_RECURRENCE", rrule);
    }
    std::system("python3 -m calendar_integration.gcal_service");
    unsetEnv("GCAL_ACTION");
    unsetEnv("GCAL_CREDS");
    unsetEnv("GCAL_CALENDAR_ID");
    unsetEnv("GCAL_TITLE");
    unsetEnv("GCAL_DESC");
    unsetEnv("GCAL_EVENT_ID");
    unsetEnv("GCAL_START");
    unsetEnv("GCAL_END");
    unsetEnv("GCAL_TZ");
    unsetEnv("GCAL_RECURRENCE");
}

