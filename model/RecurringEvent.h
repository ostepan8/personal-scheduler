#include "Event.h"
#include "recurrence/RecurrencePattern.h"
class RecurringEvent : public Event
{
private:
    RecurrencePattern &recurrencePattern;

public:
    RecurringEvent(string &id, string &desc,
                   string &title, chrono::system_clock::time_point time,
                   chrono::system_clock::duration duration, RecurrencePattern &recurrencePattern);
    vector<chrono::system_clock::time_point> getNextNOccurrences(chrono::system_clock::time_point after,
                                                                 int n) const;
    bool isDueOn(chrono::system_clock::time_point date) const;
};