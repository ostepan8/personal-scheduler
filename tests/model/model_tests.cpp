#include <cassert>
#include <iostream>
#include "../../model/Model.h"
#include "../../model/OneTimeEvent.h"
#include "../../model/RecurringEvent.h"
#include "../../model/recurrence/DailyRecurrence.h"
#include "../../model/recurrence/WeeklyRecurrence.h"
#include "../test_utils.h"

using namespace std;
using namespace chrono;

static void testModelAddAndRetrieve()
{
    Model m({});
    OneTimeEvent e1("1","d1","t1", makeTime(2025,1,2,9), hours(1));
    OneTimeEvent e2("2","d2","t2", makeTime(2025,1,1,8), hours(1));
    m.addEvent(e1);
    m.addEvent(e2);
    auto next = m.getNextEvent();
    assert(next.getId() == "2");
    auto all = m.getEvents(-1, makeTime(2025,1,3,0));
    assert(all.size() == 2);
    assert(all[0].getId() == "2");
    assert(all[1].getId() == "1");
}

static void testModelRemove()
{
    Model m({});
    OneTimeEvent e1("1","d1","t1", makeTime(2025,1,1,9), hours(1));
    OneTimeEvent e2("2","d2","t2", makeTime(2025,1,2,9), hours(1));
    m.addEvent(e1);
    m.addEvent(e2);
    assert(m.removeEvent("1"));
    assert(!m.removeEvent("3"));
    auto all = m.getEvents(-1, makeTime(2025,1,3,0));
    assert(all.size() == 1 && all[0].getId() == "2");
}

static void testModelGetEventsLimit()
{
    Model m({});
    OneTimeEvent e1("1","d1","t1", makeTime(2025,1,1,8), hours(1));
    OneTimeEvent e2("2","d2","t2", makeTime(2025,1,2,8), hours(1));
    OneTimeEvent e3("3","d3","t3", makeTime(2025,1,3,8), hours(1));
    m.addEvent(e1); m.addEvent(e2); m.addEvent(e3);
    auto limited = m.getEvents(2, makeTime(2025,1,4,0));
    assert(limited.size() == 2);
    auto cutoff = m.getEvents(-1, makeTime(2025,1,2,12));
    assert(cutoff.size() == 2);
}

static void testModelWithDailyRecurring()
{
    Model m({});
    auto start = makeTime(2025,6,1,9);
    DailyRecurrence rec(start, 1);
    string id("R");
    string desc("d");
    string title("t");
    RecurringEvent r(id, desc, title, start, hours(1), rec);
    OneTimeEvent o("O","d","t", makeTime(2025,6,2,9), hours(1));
    m.addEvent(o);
    m.addEvent(r);

    auto next = m.getNextEvent();
    assert(next.getId() == "R");

    auto evs = m.getEvents(-1, makeTime(2025,6,3,0));
    assert(evs.size() == 2);
    assert(evs[0].getId() == "R");
    assert(evs[1].getId() == "O");
}

int main()
{
    testModelAddAndRetrieve();
    testModelRemove();
    testModelGetEventsLimit();
    testModelWithDailyRecurring();
    cout << "Model tests passed\n";
    return 0;
}
