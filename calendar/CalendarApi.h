#pragma once
#include "../model/Event.h"
#include <memory>

class CalendarApi
{
public:
    virtual ~CalendarApi() = default;
    virtual void addEvent(const Event &e) = 0;
    virtual void deleteEvent(const Event &e) = 0;

    // Helper method to update by delete + add
    virtual void updateEvent(const Event &oldEvent, const Event &newEvent)
    {
        deleteEvent(oldEvent);
        addEvent(newEvent);
    }
};