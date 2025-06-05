#include "RecurrencePattern.h"
#include <chrono>
#include <vector>

// This pattern handles the "Every x days" recurrence
// Example: every 7 days I want to order Nandos.
// Example: every other day, I want to go to the gym.
class DailyRecurrence : public RecurrencePattern
{
private:
    chrono::system_clock::time_point startingPoint; // First date
    int repeatingInterval;                          // This is the x in every x days.
    int maxOccurrences;                             // optional: stop after N times (-1 = unlimited)
    chrono::system_clock::time_point endDate;       // optional: stop by a certain date
public:
    DailyRecurrence(chrono::system_clock::time_point start,
                    int interval,
                    int maxOccurrences = -1,
                    chrono::system_clock::time_point endDate = chrono::system_clock::time_point::max());

    std::vector<chrono::system_clock::time_point> getNextNOccurrences(
        chrono::system_clock::time_point after, int n) const override;

    bool isDueOn(chrono::system_clock::time_point date) const override;

    ~DailyRecurrence() override = default;
};
