#include "GoogleCalendarApi.h"
#include "../utils/TimeUtils.h"
#include "../model/RecurringEvent.h"
#include <cstdlib>
#include <string>

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

void GoogleCalendarApi::addEvent(const Event &e) {
    const int maxOcc = 25;
    if (e.isRecurring()) {
        auto *re = dynamic_cast<const RecurringEvent *>(&e);
        if (re) {
            auto times = re->getNextNOccurrences(re->getTime() - std::chrono::seconds(1), maxOcc);
            int idx = 0;
            for (auto t : times) {
                setEnv("GCAL_ACTION", "add");
                setEnv("GCAL_CREDS", credentials_);
                setEnv("GCAL_CALENDAR_ID", calendarId_);
                setEnv("GCAL_TITLE", e.getTitle());
                setEnv("GCAL_DESC", e.getDescription());
                setEnv("GCAL_EVENT_ID", e.getId() + "_" + std::to_string(idx++));
                auto start = TimeUtils::formatRFC3339UTC(t);
                auto end = TimeUtils::formatRFC3339UTC(t + e.getDuration());
                setEnv("GCAL_START", start);
                setEnv("GCAL_END", end);
                std::system("python3 -m calendar_integration.gcal_service");
                unsetEnv("GCAL_ACTION");
                unsetEnv("GCAL_CREDS");
                unsetEnv("GCAL_CALENDAR_ID");
                unsetEnv("GCAL_TITLE");
                unsetEnv("GCAL_DESC");
                unsetEnv("GCAL_EVENT_ID");
                unsetEnv("GCAL_START");
                unsetEnv("GCAL_END");
            }
            return;
        }
    }

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
    std::system("python3 -m calendar_integration.gcal_service");
    unsetEnv("GCAL_ACTION");
    unsetEnv("GCAL_CREDS");
    unsetEnv("GCAL_CALENDAR_ID");
    unsetEnv("GCAL_TITLE");
    unsetEnv("GCAL_DESC");
    unsetEnv("GCAL_EVENT_ID");
    unsetEnv("GCAL_START");
    unsetEnv("GCAL_END");
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
