#include "Event.h"
#include "recurrence/RecurrencePattern.h"
#include <chrono>
#include <vector>
#include <string>
#include <memory>
class RecurringEvent : public Event
{
private:
    std::shared_ptr<RecurrencePattern> recurrencePattern;

public:
    RecurringEvent(const string &id, const string &desc,
                   const string &title, chrono::system_clock::time_point time,
                   chrono::system_clock::duration duration, std::shared_ptr<RecurrencePattern> recurrencePattern);
    vector<chrono::system_clock::time_point> getNextNOccurrences(chrono::system_clock::time_point after,
                                                                 int n) const;
    bool isDueOn(chrono::system_clock::time_point date) const;
    std::unique_ptr<Event> clone() const override { return std::make_unique<RecurringEvent>(*this); }
};
