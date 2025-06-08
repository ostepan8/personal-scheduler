#include <cassert>
#include <cstdio>
#include <memory>
#include "../../database/SQLiteScheduleDatabase.h"
#include "../../model/Model.h"
#include "../../model/RecurringEvent.h"
#include "../../model/recurrence/DailyRecurrence.h"
#include "../test_utils.h"
#include <iostream>

using namespace std;
using namespace chrono;

static void testRecurringPersistence() {
    const char* path = "test_persist.db";
    std::remove(path);
    {
        SQLiteScheduleDatabase db(path);
        Model m(&db);
        auto start = makeTime(2025,6,9,8);
        auto pat = std::make_shared<DailyRecurrence>(start,1);
        RecurringEvent r("R","d","t", start, hours(1), pat);
        m.addEvent(r);
    }
    {
        SQLiteScheduleDatabase db(path);
        Model m(&db);
        auto events = m.getNextNEvents(3);
        assert(events.size() == 3);
        for(int i=0;i<3;i++) {
            assert(events[i].getId() == "R");
            assert(events[i].isRecurring());
        }
    }
    std::remove(path);
}

int main() {
    testRecurringPersistence();
    cout << "Database tests passed\n";
    return 0;
}
