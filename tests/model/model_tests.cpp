#include <cassert>
#include <iostream>
#include "../../model/Model.h"
#include "../../model/OneTimeEvent.h"
#include "../../model/RecurringEvent.h"
#include "../../model/recurrence/DailyRecurrence.h"
#include "../../model/recurrence/WeeklyRecurrence.h"
#include "../test_utils.h"
#include <memory>

using namespace std;
using namespace chrono;

static void testModelAddAndRetrieve()
{
    Model m;
    auto now = chrono::system_clock::now();
    OneTimeEvent e1("1","d1","t1", now + hours(2), hours(1));
    OneTimeEvent e2("2","d2","t2", now + hours(1), hours(1));
    m.addEvent(e1);
    m.addEvent(e2);
    auto next = m.getNextEvent();
    assert(next.getId() == "2");
    auto all = m.getEvents(-1, now + hours(4));
    assert(all.size() == 2);
    assert(all[0].getId() == "2");
    assert(all[1].getId() == "1");
}

static void testModelRemove()
{
    Model m;
    auto now = chrono::system_clock::now();
    OneTimeEvent e1("1","d1","t1", now + hours(1), hours(1));
    OneTimeEvent e2("2","d2","t2", now + hours(2), hours(1));
    m.addEvent(e1);
    m.addEvent(e2);
    assert(m.removeEvent("1"));
    assert(!m.removeEvent("3"));
    auto all = m.getEvents(-1, now + hours(5));
    assert(all.size() == 1 && all[0].getId() == "2");
}

static void testModelGetEventsLimit()
{
    Model m;
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
    Model m;
    auto start = makeTime(2025,6,1,9);
    auto rec = std::make_shared<DailyRecurrence>(start, 1);
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

static void testNextNWithRecurring()
{
    Model m;
    auto now = chrono::system_clock::now();
    auto start = now + hours(1);
    auto rec = std::make_shared<DailyRecurrence>(start, 1);
    RecurringEvent wake("W","wake","Wake", start, hours(1), rec);
    OneTimeEvent ball("B","play","Ball", start + hours(24*4) + hours(4), hours(2));
    m.addEvent(wake);
    m.addEvent(ball);

    auto next = m.getNextEvent();
    assert(next.getId() == "W");

    auto list = m.getNextNEvents(6);
    assert(list.size() == 6);
    for(int i=0;i<5;i++)
        assert(list[i].getTime() == start + hours(24*i));
    assert(list[5].getId() == "B");
}

static void testNextNSkipsPast()
{
    Model m;
    auto now = chrono::system_clock::now();
    auto pastStart = now - chrono::hours(24 * 2) - chrono::hours(5); // two days and 5 hours ago
    auto rec = std::make_shared<DailyRecurrence>(pastStart, 1);
    RecurringEvent past("R","d","t", pastStart, hours(1), rec);
    auto futureTime = now + chrono::hours(2);
    OneTimeEvent upcoming("F","d","t", futureTime, hours(1));
    m.addEvent(past);
    m.addEvent(upcoming);

    auto list = m.getNextNEvents(5);
    assert(!list.empty());
    // First event should be the upcoming one-time event in two hours
    assert(list.front().getId() == "F");
    for (const auto &e : list)
    {
        assert(e.getTime() > now);
    }
}

static void testEventsOnDay()
{
    Model m;
    OneTimeEvent e1("1","d","t", makeTime(2025,6,1,8), hours(1));
    OneTimeEvent e2("2","d","t", makeTime(2025,6,1,12), hours(1));
    OneTimeEvent e3("3","d","t", makeTime(2025,6,2,9), hours(1));
    m.addEvent(e1); m.addEvent(e2); m.addEvent(e3);
    auto evs = m.getEventsOnDay(makeTime(2025,6,1,0));
    assert(evs.size() == 2);
    assert(evs[0].getId() == "1");
    assert(evs[1].getId() == "2");
}

static void testEventsInWeek()
{
    Model m;
    OneTimeEvent e1("1","d","t", makeTime(2025,6,2,9), hours(1)); // Monday
    OneTimeEvent e2("2","d","t", makeTime(2025,6,5,9), hours(1)); // Thursday
    OneTimeEvent e3("3","d","t", makeTime(2025,6,9,9), hours(1)); // next Monday
    m.addEvent(e1); m.addEvent(e2); m.addEvent(e3);
    auto evs = m.getEventsInWeek(makeTime(2025,6,3,0));
    assert(evs.size() == 2);
    assert(evs[0].getId() == "1");
    assert(evs[1].getId() == "2");
}

static void testEventsInMonth()
{
    Model m;
    OneTimeEvent e1("1","d","t", makeTime(2025,6,2,9), hours(1));
    OneTimeEvent e2("2","d","t", makeTime(2025,7,1,9), hours(1));
    OneTimeEvent e3("3","d","t", makeTime(2025,6,20,9), hours(1));
    m.addEvent(e1); m.addEvent(e2); m.addEvent(e3);
    auto evs = m.getEventsInMonth(makeTime(2025,6,10,0));
    assert(evs.size() == 2);
    assert(evs[0].getId() == "1");
    assert(evs[1].getId() == "3");
}

static void testEventsTimeZones()
{
    const char *prevPtr = getenv("TZ");
    std::string prev = prevPtr ? std::string(prevPtr) : std::string();
    bool hadPrev = prevPtr != nullptr;
    setenv("TZ", "Europe/Berlin", 1);
    tzset();

    Model m({});
    OneTimeEvent e("1","d","t", makeTime(2025,6,1,9), hours(1));
    m.addEvent(e);

    auto d = m.getEventsOnDay(makeTime(2025,6,1,0));
    assert(d.size() == 1);
    auto w = m.getEventsInWeek(makeTime(2025,6,1,0));
    assert(w.size() == 1);
    auto mo = m.getEventsInMonth(makeTime(2025,6,1,0));
    assert(mo.size() == 1);

    if (hadPrev)
        setenv("TZ", prev.c_str(), 1);
    else
        unsetenv("TZ");
    tzset();
}

int main()
{
    testModelAddAndRetrieve();
    testModelRemove();
    testModelGetEventsLimit();
    testModelWithDailyRecurring();
    testNextNWithRecurring();
    testNextNSkipsPast();
    testEventsOnDay();
    testEventsInWeek();
    testEventsInMonth();
    testEventsTimeZones();
    cout << "Model tests passed\n";
    return 0;
}
