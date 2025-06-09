#pragma once
#include "../model/Event.h"
#include <memory>

class CalendarApi {
public:
    virtual ~CalendarApi() = default;
    virtual void addEvent(const Event &e) = 0;
    virtual void deleteEvent(const Event &e) = 0;
};
