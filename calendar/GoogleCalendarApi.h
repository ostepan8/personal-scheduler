#pragma once
#include "CalendarApi.h"
#include <string>

class GoogleCalendarApi : public CalendarApi {
    std::string credentials_;
    std::string calendarId_;
public:
    GoogleCalendarApi(std::string creds, std::string calendarId = "primary");
    void addEvent(const Event &e) override;
    void deleteEvent(const Event &e) override;
    void updateEvent(const Event &oldEvent, const Event &newEvent) override;
};
