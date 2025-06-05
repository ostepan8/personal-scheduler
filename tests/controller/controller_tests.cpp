#include <cassert>
#include <iostream>
#include <sstream>
#include "../../model/Model.h"
#include "../../model/OneTimeEvent.h"
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

int main()
{
    testControllerParseFormat();
    testControllerPrintNextEvent();
    testControllerPrintNextEventNone();
    cout << "Controller tests passed\n";
    return 0;
}
