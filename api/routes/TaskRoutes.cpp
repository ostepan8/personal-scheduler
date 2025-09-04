#include "TaskRoutes.h"
#include "Serialization.h"
#include "../../utils/TimeUtils.h"
#include "../../utils/Sanitize.h"
#include "../../utils/NotificationRegistry.h"
#include "../../utils/ActionRegistry.h"
#include "../../scheduler/ScheduledTask.h"
#include <iostream>
#include <algorithm>

using namespace std::chrono;
using ApiSerialization::eventToJson;

namespace TaskRoutes
{

    static std::chrono::minutes parseDuration(const std::string &t)
    {
        std::string s = t;
        s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
        if (s.empty())
            return std::chrono::minutes(0);
        char suf = s.back();
        int val;
        if (suf == 'h' || suf == 'H')
        {
            val = std::stoi(s.substr(0, s.size() - 1));
            return std::chrono::minutes(val * 60);
        }
        if (suf == 'm' || suf == 'M')
        {
            val = std::stoi(s.substr(0, s.size() - 1));
            return std::chrono::minutes(val);
        }
        val = std::stoi(s);
        return std::chrono::minutes(val);
    }

    void registerRoutes(httplib::Server &server, Model &model, EventLoop *loop)
    {
        std::cout << "Registering task routes..." << std::endl;

        server.Get("/notifiers", [](const httplib::Request &, httplib::Response &res)
                   {
        std::cout << "GET /notifiers request received" << std::endl;
        nlohmann::json out;
        try {
            auto names = NotificationRegistry::availableNotifiers();
            std::cout << "Available notifiers: " << names.size() << std::endl;
            out["status"] = "ok";
            out["data"] = names;
        } catch (const std::exception &e) {
            std::cout << "Error in /notifiers: " << e.what() << std::endl;
            out = {{"status","error"},{"message","Invalid input"}};
        }
        res.set_content(out.dump(), "application/json"); });

        server.Get("/actions", [](const httplib::Request &, httplib::Response &res)
                   {
        std::cout << "GET /actions request received" << std::endl;
        nlohmann::json out;
        try {
            auto names = ActionRegistry::availableActions();
            std::cout << "Available actions: " << names.size() << std::endl;
            out["status"] = "ok";
            out["data"] = names;
        } catch (const std::exception &e) {
            std::cout << "Error in /actions: " << e.what() << std::endl;
            out = {{"status","error"},{"message","Invalid input"}};
        }
        res.set_content(out.dump(), "application/json"); });

        server.Get("/tasks", [&model](const httplib::Request &, httplib::Response &res)
                   {
        std::cout << "GET /tasks request received" << std::endl;
        nlohmann::json out;
        try {
            auto horizon = system_clock::now() + hours(24 * 365 * 5);
            auto events = model.getEvents(-1, horizon);
            std::cout << "Retrieved " << events.size() << " events from model" << std::endl;
            nlohmann::json data = nlohmann::json::array();
            int taskCount = 0;
            for (const auto &ev : events) {
                if (ev.getCategory() == "task") {
                    data.push_back(eventToJson(ev));
                    taskCount++;
                }
            }
            std::cout << "Found " << taskCount << " tasks" << std::endl;
            out["status"] = "ok";
            out["data"] = data;
        } catch (const std::exception &e) {
            std::cout << "Error in /tasks: " << e.what() << std::endl;
            out = {{"status","error"},{"message","Invalid input"}};
        }
        res.set_content(out.dump(), "application/json"); });

        if (loop)
        {
            std::cout << "Event loop provided, registering POST /tasks endpoint" << std::endl;
            server.Post("/tasks", [loop, &model](const httplib::Request &req, httplib::Response &res)
                        {
            std::cout << "POST /tasks request received with body: " << req.body << std::endl;
            nlohmann::json out;
            try {
                auto body = nlohmann::json::parse(req.body);
                std::string title = sanitize(body.value("title", ""));
                std::string description = sanitize(body.value("description", ""), 500);
                std::string timeStr = body.at("time");
                std::string notifierName = body.at("notifier");
                std::string actionName = body.at("action");
                
                std::cout << "Creating task: " << title << ", time: " << timeStr 
                          << ", notifier: " << notifierName << ", action: " << actionName << std::endl;
                
                auto time = TimeUtils::parseTimePoint(timeStr);
                
                std::vector<std::chrono::system_clock::duration> notifyDur;
                if (body.contains("notify") && body["notify"].is_array()) {
                    for (const auto &el : body["notify"]) {
                        auto duration = parseDuration(el.get<std::string>());
                        notifyDur.push_back(duration);
                        std::cout << "Added notification " << el.get<std::string>() 
                                  << " (" << duration.count() << " minutes)" << std::endl;
                    }
                    if (notifyDur.empty()) {
                        std::cout << "No notifications specified, adding default (10 minutes)" << std::endl;
                        notifyDur.push_back(std::chrono::minutes(10));
                    }
                } else {
                    std::cout << "No notifications array, adding default (10 minutes)" << std::endl;
                    notifyDur.push_back(std::chrono::minutes(10));
                }
                
                auto notifier = NotificationRegistry::getNotifier(notifierName);
                auto action = ActionRegistry::getAction(actionName);
                
                if (!notifier) {
                    std::cout << "Error: Invalid notifier " << notifierName << std::endl;
                    throw std::runtime_error("invalid notifier");
                }
                if (!action) {
                    std::cout << "Error: Invalid action " << actionName << std::endl;
                    throw std::runtime_error("invalid action");
                }
                
                std::string id = model.generateUniqueId();
                std::cout << "Generated ID: " << id << std::endl;
                
                auto notifyCb = [notifier,id,title](){ 
                    std::cout << "Triggering notification for task " << id << ": " << title << std::endl;
                    notifier(id,title); 
                };
                
                OneTimeEvent e(id, description, title, time, seconds(0), "task");
                auto now = system_clock::now();
                std::vector<system_clock::time_point> notifyTimes;
                
                if (e.getTime() - now >= std::chrono::minutes(10)) {
                    for (auto d : notifyDur) {
                        auto tp = e.getTime() - d;
                        if (tp > now) {
                            notifyTimes.push_back(tp);
                        }
                    }
                }
                
                std::cout << "Creating scheduled task with " << notifyTimes.size() << " notifications" << std::endl;
                auto task = std::make_shared<ScheduledTask>(id, description, title,
                                            e.getTime(), e.getDuration(), notifyTimes,
                                            notifyCb, action);
                task->setCategory("task");
                // Persist notifier/action names for re-enqueue after restart
                task->setNotifierName(notifierName);
                task->setActionName(actionName);
                loop->addTask(task);
                std::cout << "Task added to event loop" << std::endl;
                
                out["status"] = "ok";
                out["data"] = eventToJson(e);
            } catch (const std::exception &e) {
                std::cout << "Error creating task: " << e.what() << std::endl;
                out = {{"status","error"},{"message","Invalid input"}};
            }
            res.set_content(out.dump(), "application/json"); });
        }
        else
        {
            std::cout << "Warning: No event loop provided, POST /tasks endpoint not registered" << std::endl;
        }

        std::cout << "Task routes registered successfully" << std::endl;
    }

} // namespace TaskRoutes
