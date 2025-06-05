#include "RecurringEvent.h"

RecurringEvent::RecurringEvent(string &id, string &desc,
                               string &title, chrono::system_clock::time_point time,
                               chrono::system_clock::duration duration,
                               RecurrencePattern &recurrencePattern)
    : Event(id, desc, title, time, duration), recurrencePattern(recurrencePattern)
{
}

bool RecurringEvent::isDueOn(chrono::system_clock::time_point data) const
{
    return recurrencePattern.isDueOn(data);
}

vector<chrono::system_clock::time_point> RecurringEvent::getNextNOccurrences(chrono::system_clock::time_point after,
                                                                             int n) const
{
    return recurrencePattern.getNextNOccurrences(after, n);
}