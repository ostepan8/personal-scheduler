#include "WeeklyRecurrence.h"
#include "../../utils/WeekDay.h"

WeeklyRecurrence::WeeklyRecurrence(
    vector<Weekday> daysOfTheWeek,
    int repeatingInterval,
    int maxOccurrences,
    chrono::system_clock::time_point
        endDate) {
    // TODO MAYBE NEED CONSTRUCTOR
};

vector<chrono::system_clock::time_point> WeeklyRecurrence::getNextNOccurrences(
    chrono::system_clock::time_point after, int n) const
{
    // TODO
}

bool WeeklyRecurrence::isDueOn(chrono::system_clock::time_point date) const
{
    // TODO
}