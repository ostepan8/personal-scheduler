#include <cassert>
#include <thread>
#include "../../api/ApiServer.h"
#include "../../model/Model.h"
#include "../../model/OneTimeEvent.h"
#include "../test_utils.h"
#include "../../external/json/nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;
using namespace chrono;

static void runServer(ApiServer &srv) {
    srv.start();
}

static void testDayEndpoint() {
    Model m({});
    OneTimeEvent e1("1","d","t", makeTime(2025,6,1,9), hours(1));
    m.addEvent(e1);
    ApiServer srv(m, 8085);
    thread th(runServer, std::ref(srv));
    this_thread::sleep_for(milliseconds(100));

    httplib::Client cli("localhost", 8085);
    auto res = cli.Get("/events/day/2025-06-01");
    assert(res && res->status == 200);
    auto j = json::parse(res->body);
    assert(j["status"] == "ok");
    assert(j["data"].size() == 1);

    srv.stop();
    th.join();
}

static void testWeekEndpoint() {
    Model m({});
    OneTimeEvent e1("1","d","t", makeTime(2025,6,2,9), hours(1)); // Monday
    OneTimeEvent e2("2","d","t", makeTime(2025,6,8,10), hours(1)); // Sunday
    m.addEvent(e1); m.addEvent(e2);
    ApiServer srv(m, 8086);
    thread th(runServer, std::ref(srv));
    this_thread::sleep_for(milliseconds(100));

    httplib::Client cli("localhost", 8086);
    auto res = cli.Get("/events/week/2025-06-03");
    assert(res && res->status == 200);
    auto j = json::parse(res->body);
    assert(j["status"] == "ok");
    assert(j["data"].size() == 2);

    srv.stop();
    th.join();
}

static void testMonthEndpoint() {
    Model m({});
    OneTimeEvent e1("1","d","t", makeTime(2025,6,2,9), hours(1));
    OneTimeEvent e2("2","d","t", makeTime(2025,7,1,9), hours(1));
    m.addEvent(e1); m.addEvent(e2);
    ApiServer srv(m, 8087);
    thread th(runServer, std::ref(srv));
    this_thread::sleep_for(milliseconds(100));

    httplib::Client cli("localhost", 8087);
    auto res = cli.Get("/events/month/2025-06");
    assert(res && res->status == 200);
    auto j = json::parse(res->body);
    assert(j["status"] == "ok");
    assert(j["data"].size() == 1);

    srv.stop();
    th.join();
}

int main() {
    testDayEndpoint();
    testWeekEndpoint();
    testMonthEndpoint();
    cout << "API tests passed\n";
    return 0;
}
