#include "ApiServer.h"
#include "../model/OneTimeEvent.h"
#include "../model/Event.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <climits>
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
    j["category"] = e.getCategory();
    return j;
}

static json timeSlotToJson(const TimeSlot &slot)
{
    json j;
    j["start"] = formatTimePoint(slot.start);
    j["end"] = formatTimePoint(slot.end);
    j["duration_minutes"] = slot.duration.count();
    return j;
}

void ApiServer::setupRoutes()
{
    // 1) OPTIONS preflight (respond with CORS headers exactly once)
    server_.Options(R"(.*)", [](const httplib::Request &, httplib::Response &res)
                    {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
        res.status = 200;
        res.set_content("", "text/plain"); });

    // ===== EXISTING ROUTES =====

    server_.Get("/events", [this](const httplib::Request &req, httplib::Response &res)
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

    // ===== NEW ROUTES: SEARCH AND FILTERING =====

    server_.Get("/events/search", [this](const httplib::Request &req, httplib::Response &res)
                {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "GET /events/search" << std::endl;
        json out;
        try {
            std::string query = req.get_param_value("q");
            int maxResults = -1;
            if (req.has_param("max")) {
                maxResults = std::stoi(req.get_param_value("max"));
            }
            
            auto events = model_.searchEvents(query, maxResults);
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

    server_.Get(R"(/events/range/(\d{4}-\d{2}-\d{2})/(\d{4}-\d{2}-\d{2}))",
                [this](const httplib::Request &req, httplib::Response &res)
                {
                    res.set_header("Access-Control-Allow-Origin", "*");
                    std::cout << "GET /events/range/" << req.matches[1] << "/" << req.matches[2] << std::endl;
                    json out;
                    try
                    {
                        auto start = parseDate(req.matches[1]);
                        auto end = parseDate(req.matches[2]) + hours(24); // Include entire end day

                        auto events = model_.getEventsInRange(start, end);
                        json data = json::array();
                        for (const auto &ev : events)
                        {
                            data.push_back(eventToJson(ev));
                        }
                        out["status"] = "ok";
                        out["data"] = data;
                    }
                    catch (const std::exception &ex)
                    {
                        out = json{{"status", "error"}, {"message", ex.what()}};
                    }
                    res.set_content(out.dump(), "application/json");
                });

    server_.Get("/events/duration", [this](const httplib::Request &req, httplib::Response &res)
                {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "GET /events/duration" << std::endl;
        json out;
        try {
            int minMinutes = 0;
            int maxMinutes = INT_MAX;
            
            if (req.has_param("min")) {
                minMinutes = std::stoi(req.get_param_value("min"));
            }
            if (req.has_param("max")) {
                maxMinutes = std::stoi(req.get_param_value("max"));
            }
            
            auto events = model_.getEventsByDuration(minMinutes, maxMinutes);
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

    // ===== CATEGORIES =====

    server_.Get("/categories", [this](const httplib::Request &, httplib::Response &res)
                {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "GET /categories" << std::endl;
        json out;
        try {
            auto categories = model_.getCategories();
            json data = json::array();
            for (const auto &cat : categories) {
                data.push_back(cat);
            }
            out["status"] = "ok";
            out["data"] = data;
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Get(R"(/events/category/(.+))", [this](const httplib::Request &req, httplib::Response &res)
                {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "GET /events/category/" << req.matches[1] << std::endl;
        json out;
        try {
            std::string category = req.matches[1];
            auto events = model_.getEventsByCategory(category);
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

    // ===== CONFLICTS AND FREE TIME =====

    server_.Get("/events/conflicts", [this](const httplib::Request &req, httplib::Response &res)
                {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "GET /events/conflicts" << std::endl;
        json out;
        try {
            std::string timeStr = req.get_param_value("time");
            int durationMin = 60; // default
            
            if (req.has_param("duration")) {
                durationMin = std::stoi(req.get_param_value("duration"));
            }
            
            auto time = parseTimePoint(timeStr);
            auto conflicts = model_.getConflicts(time, minutes(durationMin));
            
            json data = json::array();
            for (const auto &ev : conflicts) {
                data.push_back(eventToJson(ev));
            }
            out["status"] = "ok";
            out["has_conflicts"] = !conflicts.empty();
            out["data"] = data;
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Post("/events/validate", [this](const httplib::Request &req, httplib::Response &res)
                 {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "POST /events/validate" << std::endl;
        json out;
        try {
            auto body = json::parse(req.body);
            std::string timeStr = body.at("time");
            int durationSec = body.value("duration", 3600);
            std::string title = body.value("title", "Test Event");
            
            auto time = parseTimePoint(timeStr);
            OneTimeEvent testEvent("temp", "", title, time, seconds(durationSec));
            
            bool valid = model_.validateEventTime(testEvent);
            out["status"] = "ok";
            out["valid"] = valid;
            if (!valid) {
                auto conflicts = model_.getConflicts(time, duration_cast<minutes>(seconds(durationSec)));
                json conflictData = json::array();
                for (const auto &ev : conflicts) {
                    conflictData.push_back(eventToJson(ev));
                }
                out["conflicts"] = conflictData;
            }
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Get(R"(/free-slots/(\d{4}-\d{2}-\d{2}))", [this](const httplib::Request &req, httplib::Response &res)
                {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "GET /free-slots/" << req.matches[1] << std::endl;
        json out;
        try {
            auto date = parseDate(req.matches[1]);
            int startHour = 9;  // default
            int endHour = 17;   // default
            int minDuration = 30; // default
            
            if (req.has_param("start")) {
                startHour = std::stoi(req.get_param_value("start"));
            }
            if (req.has_param("end")) {
                endHour = std::stoi(req.get_param_value("end"));
            }
            if (req.has_param("duration")) {
                minDuration = std::stoi(req.get_param_value("duration"));
            }
            
            auto slots = model_.findFreeSlots(date, startHour, endHour, minDuration);
            json data = json::array();
            for (const auto &slot : slots) {
                data.push_back(timeSlotToJson(slot));
            }
            out["status"] = "ok";
            out["data"] = data;
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Get("/free-slots/next", [this](const httplib::Request &req, httplib::Response &res)
                {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "GET /free-slots/next" << std::endl;
        json out;
        try {
            int durationMin = 60; // default
            auto after = system_clock::now(); // default
            
            if (req.has_param("duration")) {
                durationMin = std::stoi(req.get_param_value("duration"));
            }
            if (req.has_param("after")) {
                std::string afterStr = req.get_param_value("after");
                after = parseTimePoint(afterStr);
            }
            
            auto slot = model_.findNextAvailableSlot(minutes(durationMin), after);
            out["status"] = "ok";
            out["data"] = timeSlotToJson(slot);
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    // ===== STATISTICS =====

    server_.Get(R"(/stats/events/(\d{4}-\d{2}-\d{2})/(\d{4}-\d{2}-\d{2}))",
                [this](const httplib::Request &req, httplib::Response &res)
                {
                    res.set_header("Access-Control-Allow-Origin", "*");
                    std::cout << "GET /stats/events/" << req.matches[1] << "/" << req.matches[2] << std::endl;
                    json out;
                    try
                    {
                        auto start = parseDate(req.matches[1]);
                        auto end = parseDate(req.matches[2]) + hours(24);

                        auto stats = model_.getEventStats(start, end);
                        json data;
                        data["total_events"] = stats.totalEvents;
                        data["total_minutes"] = stats.totalMinutes;
                        data["events_by_category"] = stats.eventsByCategory;

                        json busyDays = json::array();
                        for (const auto &[date, count] : stats.busiestDays)
                        {
                            json day;
                            day["date"] = formatTimePoint(date);
                            day["event_count"] = count;
                            busyDays.push_back(day);
                        }
                        data["busiest_days"] = busyDays;

                        json busyHours = json::array();
                        for (const auto &[hour, count] : stats.busiestHours)
                        {
                            json h;
                            h["hour"] = hour;
                            h["event_count"] = count;
                            busyHours.push_back(h);
                        }
                        data["busiest_hours"] = busyHours;

                        out["status"] = "ok";
                        out["data"] = data;
                    }
                    catch (const std::exception &ex)
                    {
                        out = json{{"status", "error"}, {"message", ex.what()}};
                    }
                    res.set_content(out.dump(), "application/json");
                });

    // ===== UPDATE OPERATIONS =====

    server_.Put(R"(/events/(.+))", [this](const httplib::Request &req, httplib::Response &res)
                {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "PUT /events/" << req.matches[1] << std::endl;
        json out;
        try {
            std::string id = req.matches[1];
            auto body = json::parse(req.body);
            
            std::string title = body.at("title");
            std::string description = body.value("description", "");
            std::string timeStr = body.at("time");
            int durationSec = body.value("duration", 3600);
            std::string category = body.value("category", "");
            
            auto time = parseTimePoint(timeStr);
            OneTimeEvent updated(id, description, title, time, seconds(durationSec));
            updated.setCategory(category);
            
            if (!model_.updateEvent(id, updated)) {
                throw std::runtime_error("Failed to update event");
            }
            
            out["status"] = "ok";
            out["data"] = eventToJson(updated);
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Patch(R"(/events/(.+))", [this](const httplib::Request &req, httplib::Response &res)
                  {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "PATCH /events/" << req.matches[1] << std::endl;
        json out;
        try {
            std::string id = req.matches[1];
            auto body = json::parse(req.body);
            
            std::unordered_map<std::string, std::string> fields;
            for (auto& [key, value] : body.items()) {
                if (value.is_string()) {
                    fields[key] = value.get<std::string>();
                } else {
                    fields[key] = value.dump();
                }
            }
            
            if (!model_.updateEventFields(id, fields)) {
                throw std::runtime_error("Failed to update event fields");
            }
            
            // Get updated event
            auto updated = model_.getEventById(id);
            if (updated) {
                out["status"] = "ok";
                out["data"] = eventToJson(*updated);
            } else {
                throw std::runtime_error("Event not found after update");
            }
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    // ===== BULK OPERATIONS =====

    server_.Post("/events/bulk", [this](const httplib::Request &req, httplib::Response &res)
                 {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "POST /events/bulk" << std::endl;
        json out;
        try {
            auto body = json::parse(req.body);
            auto eventsJson = body.at("events");
            
            std::vector<OneTimeEvent> events;
            for (const auto& eventData : eventsJson) {
                std::string id = model_.generateUniqueId();
                std::string title = eventData.at("title");
                std::string description = eventData.value("description", "");
                std::string timeStr = eventData.at("time");
                int durationSec = eventData.value("duration", 3600);
                
                auto time = parseTimePoint(timeStr);
                events.emplace_back(id, description, title, time, seconds(durationSec));
            }
            
            std::vector<bool> results;
            for (const auto& event : events) {
                results.push_back(model_.addEvent(event));
            }
            
            json data;
            data["total"] = events.size();
            data["successful"] = std::count(results.begin(), results.end(), true);
            data["results"] = results;
            
            out["status"] = "ok";
            out["data"] = data;
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    server_.Delete("/events/bulk", [this](const httplib::Request &req, httplib::Response &res)
                   {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "DELETE /events/bulk" << std::endl;
        json out;
        try {
            auto body = json::parse(req.body);
            auto ids = body.at("ids").get<std::vector<std::string>>();
            
            int removed = model_.removeEvents(ids);
            
            out["status"] = "ok";
            out["removed"] = removed;
            out["requested"] = ids.size();
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    // ===== SOFT DELETE AND RESTORE =====

    server_.Get("/events/deleted", [this](const httplib::Request &, httplib::Response &res)
                {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "GET /events/deleted" << std::endl;
        json out;
        try {
            auto events = model_.getDeletedEvents();
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

    server_.Post(R"(/events/(.+)/restore)", [this](const httplib::Request &req, httplib::Response &res)
                 {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "POST /events/" << req.matches[1] << "/restore" << std::endl;
        json out;
        try {
            std::string id = req.matches[1];
            bool restored = model_.restoreEvent(id);
            
            if (!restored) {
                throw std::runtime_error("Event not found in deleted events");
            }
            
            out["status"] = "ok";
            out["message"] = "Event restored successfully";
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });

    // ===== EXISTING ROUTES (day, week, month, etc.) =====

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
            std::string category = body.value("category", "");
            
            auto time = parseTimePoint(timeStr);
            OneTimeEvent e(id, description, title, time, std::chrono::seconds(durationSec), category);
            
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
            bool softDelete = req.has_param("soft") && req.get_param_value("soft") == "true";
            
            bool ok = model_.removeEvent(id, softDelete);
            if (!ok) {
                throw std::runtime_error("ID not found");
            }
            out["status"] = "ok";
            out["soft_delete"] = softDelete;
        } catch (const std::exception &ex) {
            out = json{{"status", "error"}, {"message", ex.what()}};
        }
        res.set_content(out.dump(), "application/json"); });
}