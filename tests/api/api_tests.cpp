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

static const char *API_KEY_VAL = "secret";

static void testDayEndpoint() {
    setenv("API_KEY", API_KEY_VAL, 1);
    Model m;
    OneTimeEvent e1("1","d","t", makeTime(2025,6,1,9), hours(1));
    m.addEvent(e1);
    ApiServer srv(m, 8085);
    thread th(runServer, std::ref(srv));
    this_thread::sleep_for(milliseconds(100));

    httplib::Client cli("localhost", 8085);
    httplib::Headers h{{"Authorization", API_KEY_VAL}};
    auto res = cli.Get("/events/day/2025-06-01", h);
    assert(res && res->status == 200);
    auto j = json::parse(res->body);
    assert(j["status"] == "ok");
    assert(j["data"].size() == 1);

    srv.stop();
    th.join();
}

static void testWeekEndpoint() {
    setenv("API_KEY", API_KEY_VAL, 1);
    Model m;
    OneTimeEvent e1("1","d","t", makeTime(2025,6,2,9), hours(1)); // Monday
    OneTimeEvent e2("2","d","t", makeTime(2025,6,8,10), hours(1)); // Sunday
    m.addEvent(e1); m.addEvent(e2);
    ApiServer srv(m, 8086);
    thread th(runServer, std::ref(srv));
    this_thread::sleep_for(milliseconds(100));

    httplib::Client cli("localhost", 8086);
    httplib::Headers h{{"Authorization", API_KEY_VAL}};
    auto res = cli.Get("/events/week/2025-06-03", h);
    assert(res && res->status == 200);
    auto j = json::parse(res->body);
    assert(j["status"] == "ok");
    assert(j["data"].size() == 2);

    srv.stop();
    th.join();
}

static void testMonthEndpoint() {
    setenv("API_KEY", API_KEY_VAL, 1);
    Model m;
    OneTimeEvent e1("1","d","t", makeTime(2025,6,2,9), hours(1));
    OneTimeEvent e2("2","d","t", makeTime(2025,7,1,9), hours(1));
    m.addEvent(e1); m.addEvent(e2);
    ApiServer srv(m, 8087);
    thread th(runServer, std::ref(srv));
    this_thread::sleep_for(milliseconds(100));

    httplib::Client cli("localhost", 8087);
    httplib::Headers h{{"Authorization", API_KEY_VAL}};
    auto res = cli.Get("/events/month/2025-06", h);
    assert(res && res->status == 200);
    auto j = json::parse(res->body);
    assert(j["status"] == "ok");
    assert(j["data"].size() == 1);

    srv.stop();
    th.join();
}

static void testCORSEnabled() {
    setenv("API_KEY", API_KEY_VAL, 1);
    setenv("CORS_ORIGIN", "*", 1);
    Model m;
    ApiServer srv(m, 8088);
    thread th(runServer, std::ref(srv));
    this_thread::sleep_for(milliseconds(100));

    httplib::Client cli("localhost", 8088);
    httplib::Headers h{{"Authorization", API_KEY_VAL}};
    auto res = cli.Options("/events", h);
    assert(res && res->status == 200);
    assert(res->get_header_value("Access-Control-Allow-Origin") == "*");
    assert(!res->get_header_value("Access-Control-Allow-Methods").empty());

    srv.stop();
    th.join();
}

static void testDeleteAllEndpoint() {
    setenv("API_KEY", API_KEY_VAL, 1);
    Model m;
    OneTimeEvent e1("1","d","t", makeTime(2025,6,1,9), hours(1));
    m.addEvent(e1);
    ApiServer srv(m, 8089);
    thread th(runServer, std::ref(srv));
    this_thread::sleep_for(milliseconds(100));

    httplib::Client cli("localhost", 8089);
    httplib::Headers h{{"Authorization", API_KEY_VAL}};
    auto res = cli.Delete("/events", h);
    assert(res && res->status == 200);
    auto j = json::parse(res->body);
    assert(j["status"] == "ok");

    auto res2 = cli.Get("/events", h);
    auto j2 = json::parse(res2->body);
    assert(j2["data"].size() == 0);

    srv.stop();
    th.join();
}

static void testDeleteDayEndpoint() {
    setenv("API_KEY", API_KEY_VAL, 1);
    Model m;
    OneTimeEvent e1("1","d","t", makeTime(2025,6,1,9), hours(1));
    m.addEvent(e1);
    ApiServer srv(m, 8090);
    thread th(runServer, std::ref(srv));
    this_thread::sleep_for(milliseconds(100));

    httplib::Client cli("localhost", 8090);
    httplib::Headers h{{"Authorization", API_KEY_VAL}};
    auto res = cli.Delete("/events/day/2025-06-01", h);
    assert(res && res->status == 200);
    auto j = json::parse(res->body);
    assert(j["status"] == "ok");

    auto res2 = cli.Get("/events", h);
    auto j2 = json::parse(res2->body);
    assert(j2["data"].size() == 0);

    srv.stop();
    th.join();
}

static void testDeleteBeforeEndpoint() {
    setenv("API_KEY", API_KEY_VAL, 1);
    Model m;
    OneTimeEvent e1("1","d","t", makeTime(2025,6,1,9), hours(1));
    OneTimeEvent e2("2","d","t", makeTime(2025,6,3,9), hours(1));
    m.addEvent(e1); m.addEvent(e2);
    ApiServer srv(m, 8091);
    thread th(runServer, std::ref(srv));
    this_thread::sleep_for(milliseconds(100));

    httplib::Client cli("localhost", 8091);
    httplib::Headers h{{"Authorization", API_KEY_VAL}};
    auto res = cli.Delete("/events/before/2025-06-02T00:00", h);
    assert(res && res->status == 200);
    auto j = json::parse(res->body);
    assert(j["status"] == "ok");

    auto res2 = cli.Get("/events", h);
    auto j2 = json::parse(res2->body);
    assert(j2["data"].size() == 1);

    srv.stop();
    th.join();
}

int main() {
    testDayEndpoint();
    testWeekEndpoint();
    testMonthEndpoint();
    testCORSEnabled();
    testDeleteAllEndpoint();
    testDeleteDayEndpoint();
    testDeleteBeforeEndpoint();
    cout << "API tests passed\n";
    return 0;
}
