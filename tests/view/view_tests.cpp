#include <cassert>
#include <sstream>
#include "../../model/Model.h"
#include "../../model/OneTimeEvent.h"
#include "../../model/RecurringEvent.h"
#include "../../model/recurrence/DailyRecurrence.h"
#include "../../view/TextualView.h"
#include "../test_utils.h"
#include <memory>

using namespace std;
using namespace chrono;

static void testViewShowsRecurringFlag()
{
    Model m;
    OneTimeEvent o("O","d","t", makeTime(2025,6,1,9), hours(1));
    auto start = makeTime(2025,6,2,9);
    auto pat = make_shared<DailyRecurrence>(start, 1);
    RecurringEvent r("R","d","t", start, hours(1), pat);
    m.addEvent(o);
    m.addEvent(r);
    TextualView v(m);
    stringstream ss;
    auto old = cout.rdbuf(ss.rdbuf());
    v.render();
    cout.rdbuf(old);
    string out = ss.str();
    assert(out.find("(recurring)") != string::npos);
}

int main()
{
    testViewShowsRecurringFlag();
    cout << "View tests passed\n";
    return 0;
}
