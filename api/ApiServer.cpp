#include "ApiServer.h"
#include "../model/OneTimeEvent.h"
#include "../model/Event.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include "../utils/TimeUtils.h"

using json = nlohmann::json;
using namespace std::chrono;

using TimeUtils::formatTimePoint;
using TimeUtils::parseDate;
using TimeUtils::parseMonth;
using TimeUtils::parseTimePoint;

ApiServer::ApiServer(Model &model, int port)
    : model_(model), port_(port)
{

    setupRoutes();
}

void ApiServer::start()
{
    std::cout << "Starting API server on port " << port_ << std::endl;
    server_.listen("0.0.0.0", port_);
}

void ApiServer::stop()
{
    server_.stop();
}

static json eventToJson(const Event &e)
{
    json j;
    j["id"] = e.getId();
    j["title"] = e.getTitle();
    j["description"] = e.getDescription();
    j["time"] = formatTimePoint(e.getTime());
    j["duration"] = std::chrono::duration_cast<std::chrono::seconds>(e.getDuration()).count();
    return j;
}

void ApiServer::setupRoutes()
{
    // 1) OPTIONS preflight (respond with CORS headers exactly once)
    server_.Options(R"(.*)", [](const httplib::Request &, httplib::Response &res)
                    {
        // Only one '*' allowed here:
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
        res.status = 200;
        res.set_content("", "text/plain"); });

    server_.Get("/events", [this](const httplib::Request &, httplib::Response &res)
                {
                    res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "GET /events" << std::endl;
        json out;
        try {
            auto farFuture = system_clock::now() + hours(24 * 365);
            auto events = model_.getEvents(-1, farFuture);
            json data = json::array();
            for (const auto &ev : events) {
                data.push_back(eventToJson(ev));
            }
            out["status"] = "ok";
            out["data"] = data;
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Get("/events/next", [this](const httplib::Request &, httplib::Response &res)
                {
                    res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "GET /events/next" << std::endl;
        json out;
        try {
            auto ev = model_.getNextEvent();
            out["status"] = "ok";
            out["data"] = eventToJson(ev);
        } catch (const std::runtime_error &) {
            // When there are 0 events, return status ok with empty data
            out["status"] = "ok";
            out["data"] = nullptr;
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Get(R"(/events/day/(\d{4}-\d{2}-\d{2}))", [this](const httplib::Request &req, httplib::Response &res)
                {
                    res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "GET /events/day/" << req.matches[1] << std::endl;
        json out;
        try {
            auto day = parseDate(req.matches[1]);
            auto events = model_.getEventsOnDay(day);
            json data = json::array();
            for (const auto &ev : events) data.push_back(eventToJson(ev));
            out["status"] = "ok";
            out["data"] = data;
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Get(R"(/events/week/(\d{4}-\d{2}-\d{2}))", [this](const httplib::Request &req, httplib::Response &res)
                {
                    res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "GET /events/week/" << req.matches[1] << std::endl;
        json out;
        try {
            auto day = parseDate(req.matches[1]);
            auto events = model_.getEventsInWeek(day);
            json data = json::array();
            for (const auto &ev : events) data.push_back(eventToJson(ev));
            out["status"] = "ok";
            out["data"] = data;
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Get(R"(/events/month/(\d{4}-\d{2}))", [this](const httplib::Request &req, httplib::Response &res)
                {
                    res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "GET /events/month/" << req.matches[1] << std::endl;
        json out;
        try {
            auto month = parseMonth(req.matches[1]);
            auto events = model_.getEventsInMonth(month);
            json data = json::array();
            for (const auto &ev : events) data.push_back(eventToJson(ev));
            out["status"] = "ok";
            out["data"] = data;
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Post("/events", [this](const httplib::Request &req, httplib::Response &res)
                 {
                    res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "POST /events" << std::endl;
        json out;
        try {
            auto body = json::parse(req.body);
            std::string id = model_.generateUniqueId();
            std::string title = body.value("title", "");
            std::string description = body.value("description", "");
            std::string timeStr = body.at("time");
            int durationSec = body.value("duration", 0);
            auto time = parseTimePoint(timeStr);
            OneTimeEvent e(id, description, title, time, std::chrono::seconds(durationSec));
            if (!model_.addEvent(e)) {
                throw std::runtime_error("Failed to add event");
            }
            out["status"] = "ok";
            out["data"] = eventToJson(e);
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Delete("/events", [this](const httplib::Request &, httplib::Response &res)
                   {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "DELETE /events" << std::endl;
        json out;
        try {
            model_.removeAllEvents();
            out["status"] = "ok";
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Delete(R"(/events/day/(\d{4}-\d{2}-\d{2}))", [this](const httplib::Request &req, httplib::Response &res)
                   {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "DELETE /events/day/" << req.matches[1] << std::endl;
        json out;
        try {
            auto day = parseDate(req.matches[1]);
            int n = model_.removeEventsOnDay(day);
            out["status"] = "ok";
            out["removed"] = n;
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Delete(R"(/events/week/(\d{4}-\d{2}-\d{2}))", [this](const httplib::Request &req, httplib::Response &res)
                   {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "DELETE /events/week/" << req.matches[1] << std::endl;
        json out;
        try {
            auto day = parseDate(req.matches[1]);
            int n = model_.removeEventsInWeek(day);
            out["status"] = "ok";
            out["removed"] = n;
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Delete(R"(/events/before/(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}))", [this](const httplib::Request &req, httplib::Response &res)
                   {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "DELETE /events/before/" << req.matches[1] << std::endl;
        json out;
        try {
            std::string ts = req.matches[1];
            for (auto &c : ts) if (c == 'T') c = ' ';
            auto tp = parseTimePoint(ts);
            int n = model_.removeEventsBefore(tp);
            out["status"] = "ok";
            out["removed"] = n;
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Delete(R"(/events/(.+))", [this](const httplib::Request &req, httplib::Response &res)
                   {
                    res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "DELETE /events/" << req.matches[1] << std::endl;
        json out;
        try {
            std::string id = req.matches[1];
            bool ok = model_.removeEvent(id);
            if (!ok) {
                throw std::runtime_error("ID not found");
            }
            out["status"] = "ok";
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });
}
