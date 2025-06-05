#pragma once
#include <vector>
using namespace std;

// This is a recurrence pattern which the RecurringEvent will be composed with.
// This set up will make it easier to handle recurring events.
class RecurrencePattern
{
public:
    virtual vector<chrono::system_clock::time_point> getNextNOccurrences(chrono::system_clock::time_point after,
                                                                         int n) const = 0;
    virtual bool isDueOn(chrono::system_clock::time_point date) const = 0;
    virtual ~RecurrencePattern() = default;
};