#include "DailyRecurrence.h"

DailyRecurrence::DailyRecurrence(
    chrono::system_clock::time_point start,
    int interval,
    int maxOccurrences,
    chrono::system_clock::time_point endDate) {
    // TODO MAYBE NEED CONSTRUCTOR
};

vector<chrono::system_clock::time_point> DailyRecurrence::getNextNOccurrences(
    chrono::system_clock::time_point after, int n) const
{
    // TODO
}

bool DailyRecurrence::isDueOn(chrono::system_clock::time_point date) const
{
    // TODO
}