#include "RecurringEvent.h"
#include <memory>

RecurringEvent::RecurringEvent(const string &id, const string &desc,
                               const string &title, chrono::system_clock::time_point time,
                               chrono::system_clock::duration duration,
                               std::shared_ptr<RecurrencePattern> recurrencePattern)
    : Event(id, desc, title, time, duration), recurrencePattern(std::move(recurrencePattern))
{
    setRecurring(true);
}

bool RecurringEvent::isDueOn(chrono::system_clock::time_point data) const
{
    return recurrencePattern->isDueOn(data);
}

vector<chrono::system_clock::time_point> RecurringEvent::getNextNOccurrences(chrono::system_clock::time_point after,
                                                                             int n) const
{
    return recurrencePattern->getNextNOccurrences(after, n);
}
