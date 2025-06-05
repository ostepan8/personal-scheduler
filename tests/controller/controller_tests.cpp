#include <cassert>
#include <iostream>
#include <sstream>
#include "../../model/Model.h"
#include "../../model/OneTimeEvent.h"
#include "../../model/RecurringEvent.h"
#include "../../model/recurrence/DailyRecurrence.h"
#include "../test_utils.h"

#define private public
#include "../../controller/Controller.h"
#undef private

using namespace std;
using namespace chrono;

static void testControllerParseFormat()
{
    auto tp = Controller::parseTimePoint("2025-06-01 09:30");
    string s = Controller::formatTimePoint(tp);
    assert(s == "2025-06-01 09:30");

    bool threw = false;
    try { Controller::parseTimePoint("bad format"); }
    catch(const exception&) { threw = true; }
    assert(threw);

    const char* prev = getenv("TZ");
    setenv("TZ", "Europe/Berlin", 1);
    tzset();
    auto tp2 = Controller::parseTimePoint("2025-06-05 12:00");
    string s2 = Controller::formatTimePoint(tp2);
    assert(s2 == "2025-06-05 12:00");
    if (prev)
        setenv("TZ", prev, 1);
    else
        unsetenv("TZ");
    tzset();
}

static void testControllerParseFormatTimeZones()
{
    struct Case { const char* zone; const char* utc; } cases[] = {
        {"UTC", "2025-06-05 12:00"},
        {"Europe/London", "2025-06-05 11:00"},
        {"America/Chicago", "2025-06-05 17:00"},
        {"Asia/Tokyo", "2025-06-05 03:00"}
    };

    for (const auto &c : cases)
    {
        const char* prev = getenv("TZ");
        setenv("TZ", c.zone, 1);
        tzset();

        auto tp = Controller::parseTimePoint("2025-06-05 12:00");
        string round = Controller::formatTimePoint(tp);
        assert(round == "2025-06-05 12:00");

        time_t t = system_clock::to_time_t(tp);
        tm gm;
        gmtime_r(&t, &gm);
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &gm);
        string utc = buf;
        assert(utc == c.utc);

        if (prev)
            setenv("TZ", prev, 1);
        else
            unsetenv("TZ");
        tzset();
    }
}

static void testControllerCrossTimeZones()
{
    const char* prev = getenv("TZ");

    // Event created in Chicago during DST
    setenv("TZ", "America/Chicago", 1);
    tzset();
    auto tpSummer = Controller::parseTimePoint("2025-06-01 14:00");

    // View the event from different locations
    string chicago = Controller::formatTimePoint(tpSummer);
    assert(chicago == "2025-06-01 14:00");

    setenv("TZ", "Europe/London", 1);
    tzset();
    string london = Controller::formatTimePoint(tpSummer);
    assert(london == "2025-06-01 20:00");

    setenv("TZ", "Asia/Tokyo", 1);
    tzset();
    string tokyo = Controller::formatTimePoint(tpSummer);
    assert(tokyo == "2025-06-02 04:00");

    // Event created in Chicago outside of DST
    setenv("TZ", "America/Chicago", 1);
    tzset();
    auto tpWinter = Controller::parseTimePoint("2025-01-15 14:00");
    string chicagoW = Controller::formatTimePoint(tpWinter);
    assert(chicagoW == "2025-01-15 14:00");

    setenv("TZ", "Europe/London", 1);
    tzset();
    string londonW = Controller::formatTimePoint(tpWinter);
    assert(londonW == "2025-01-15 20:00");

    setenv("TZ", "Asia/Tokyo", 1);
    tzset();
    string tokyoW = Controller::formatTimePoint(tpWinter);
    assert(tokyoW == "2025-01-16 05:00");

    if (prev)
        setenv("TZ", prev, 1);
    else
        unsetenv("TZ");
    tzset();
}

static void testControllerPrintNextEvent()
{
    Model m({});
    OneTimeEvent e1("1","d","t", makeTime(2025,6,1,9), hours(1));
    m.addEvent(e1);
    StubView v(m);
    Controller c(m,v);

    std::ostringstream ss;
    auto old = cout.rdbuf(ss.rdbuf());
    c.printNextEvent();
    cout.rdbuf(old);
    assert(ss.str().find("[1]") != string::npos);
}

static void testControllerPrintNextEventNone()
{
    Model m({});
    StubView v(m);
    Controller c(m,v);
    std::ostringstream ss;
    auto old = cout.rdbuf(ss.rdbuf());
    c.printNextEvent();
    cout.rdbuf(old);
    assert(ss.str().find("no upcoming events") != string::npos);
}

static void testControllerAddRecurring()
{
    Model m({});
    StubView v(m);
    Controller c(m,v);

    auto start = makeTime(2025,6,1,9);
    DailyRecurrence pattern(start, 1);
    c.addRecurringEvent("R", "t", "d", start, hours(1), pattern);

    OneTimeEvent o("O","d","t", makeTime(2025,6,2,9), hours(1));
    m.addEvent(o);

    std::ostringstream ss;
    auto old = cout.rdbuf(ss.rdbuf());
    c.printNextEvent();
    cout.rdbuf(old);
    assert(ss.str().find("[R]") != string::npos);
}

int main()
{
    testControllerParseFormat();
    testControllerParseFormatTimeZones();
    testControllerCrossTimeZones();
    testControllerPrintNextEvent();
    testControllerPrintNextEventNone();
    testControllerAddRecurring();
    cout << "Controller tests passed\n";
    return 0;
}
