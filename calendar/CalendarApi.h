#pragma once

#include "../model/Event.h"

class CalendarApi
{
public:
    virtual ~CalendarApi() = default;

    // Add a new event to the calendar
    virtual void addEvent(const Event &event) = 0;

    // Update an existing event
    virtual void updateEvent(const Event &oldEvent, const Event &newEvent) = 0;

    // Delete an event from the calendar
    virtual void deleteEvent(const Event &event) = 0;
};