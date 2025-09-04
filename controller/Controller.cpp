// Controller.cpp
#include "Controller.h"
#include "../model/OneTimeEvent.h"
#include "../model/RecurringEvent.h"
#include "../model/recurrence/DailyRecurrence.h"
#include "../model/recurrence/WeeklyRecurrence.h"
#include "../utils/WeekDay.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cctype>
#include "../utils/TimeUtils.h"
#include <stdexcept>
#include "../scheduler/ScheduledTask.h"
#include "../utils/ActionRegistry.h"
#include "../utils/BuiltinActions.h"
#include "../utils/NotificationRegistry.h"
#include "../utils/BuiltinNotifiers.h"
#include <memory>

using namespace std;
using namespace std::chrono;

Controller::Controller(Model &model, View &view, EventLoop *loop)
    : model_(model), view_(view), loop_(loop)
{
}


void Controller::printNextEvent()
{
    try
    {
        Event next = model_.getNextEvent();
        cout << "Next event: ["
             << next.getId() << "] \""
             << next.getTitle() << "\" @ "
            << TimeUtils::formatTimePoint(next.getTime())
             << "\n";
    }
    catch (const exception &)
    {
        cout << "(no upcoming events)\n";
    }
}

string Controller::addRecurringEvent(const string &title,
                                     const string &desc,
                                     system_clock::time_point start,
                                     system_clock::duration dur,
                                     std::shared_ptr<RecurrencePattern> pattern)
{
    string idCopy = model_.generateUniqueId();
    string descCopy = desc;
    string titleCopy = title;
    RecurringEvent e(idCopy, descCopy, titleCopy, start, dur, std::move(pattern));
    model_.addEvent(e);
    return idCopy;
}

void Controller::run()
{
    cout << "=== Scheduler CLI ===\n";
    cout << "(All times are entered and displayed in local time,\n"
            " but stored internally in UTC.)\n";

    // Register built-in actions. Additional actions can be registered
    // elsewhere before Controller::run is invoked.
    BuiltinActions::registerAll();
    BuiltinNotifiers::registerAll();


    using Cmd = std::function<void()>;
    std::unordered_map<std::string, Cmd> commands;

    commands["add"] = [&]() {
        string id = model_.generateUniqueId();
        cout << "Enter title: ";
        string title; getline(cin, title);
        cout << "Enter description: ";
        string desc; getline(cin, desc);
        cout << "Enter hours from now (integer): ";
        int hrs; cin >> hrs; cin.ignore(numeric_limits<streamsize>::max(), '\n');
        auto now = system_clock::now();
        auto tp = now + hours(hrs);
        OneTimeEvent e{id, desc, title, tp, hours(1)};
        model_.addEvent(e);
        scheduleTask(e);
        cout << "Added event [" << id << "]\n";
    };

    commands["addat"] = [&]() {
        string id = model_.generateUniqueId();
        cout << "Enter title: ";
        string title; getline(cin, title);
        cout << "Enter description: ";
        string desc; getline(cin, desc);
        cout << "Enter time (YYYY-MM-DD HH:MM): ";
        string timestr; getline(cin, timestr);
        system_clock::time_point tp;
        try { tp = TimeUtils::parseTimePoint(timestr); }
        catch(const exception &e) { cout << e.what() << "\n"; return; }
        OneTimeEvent e{id, desc, title, tp, hours(1)};
        model_.addEvent(e);
        scheduleTask(e);
        cout << "Added event [" << id << "]\n";
    };

    commands["addrec"] = [&]() {
        string title, desc, timestr, rtype;
        cout << "Enter title: "; getline(cin, title);
        cout << "Enter description: "; getline(cin, desc);
        cout << "Enter start time (YYYY-MM-DD HH:MM): "; getline(cin, timestr);
        system_clock::time_point start;
        try { start = TimeUtils::parseTimePoint(timestr); }
        catch(const exception &e) { cout << e.what() << "\n"; return; }
        // Ask for duration (minutes)
        cout << "Enter duration in minutes (default 60): ";
        string durStr; getline(cin, durStr);
        int durMin = 60;
        if (!durStr.empty()) {
            try { durMin = stoi(durStr); }
            catch(const exception &) { cout << "Invalid duration, using 60 minutes.\n"; durMin = 60; }
            if (durMin <= 0) { cout << "Duration must be positive, using 60 minutes.\n"; durMin = 60; }
        }
        cout << "Recurrence type (daily/weekly): "; getline(cin, rtype);
        shared_ptr<RecurrencePattern> pat;
        if (rtype == "weekly" || rtype == "w") {
            cout << "Interval in weeks: "; int weeks; cin >> weeks;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Days of week (0=Sun..6=Sat comma separated): ";
            string daysStr; getline(cin, daysStr);
            vector<Weekday> days; string tmp; istringstream ds(daysStr);
            while (getline(ds,tmp,',')) { int d=stoi(tmp); days.push_back(static_cast<Weekday>(d)); }
            pat = make_shared<WeeklyRecurrence>(start, days, weeks);
        } else {
            cout << "Interval in days: "; int days; cin >> days;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            pat = make_shared<DailyRecurrence>(start, days);
        }
        string id = addRecurringEvent(title, desc, start, std::chrono::minutes(durMin), pat);
        cout << "Added recurring event [" << id << "]\n";
    };

    commands["addtask"] = [&]() {
        if (!loop_) { std::cout << "(event loop not running)\n"; return; }
        string title, desc, timestr, notifierName, actionName;
        cout << "Enter title: "; getline(cin, title);
        cout << "Enter description: "; getline(cin, desc);
        cout << "Enter time (YYYY-MM-DD HH:MM): "; getline(cin, timestr);
        system_clock::time_point tp;
        try { tp = TimeUtils::parseTimePoint(timestr); } catch(const exception &e) {
            cout << e.what() << "\n"; return; }
        auto notifiers = NotificationRegistry::availableNotifiers();
        if(notifiers.empty()) {
            cout << "No notifiers registered\n"; return; }
        cout << "Available notifiers:";
        for(const auto &n : notifiers) cout << " " << n;
        cout << "\nEnter notifier name: ";
        getline(cin, notifierName);
        auto note = NotificationRegistry::getNotifier(notifierName);
        if(!note) { cout << "Unknown notifier\n"; return; }

        auto actions = ActionRegistry::availableActions();
        if(actions.empty()) {
            cout << "No actions registered\n"; return; }
        cout << "Available actions:";
        for(const auto &n : actions) cout << " " << n;
        cout << "\nEnter action name: ";
        getline(cin, actionName);
        auto act = ActionRegistry::getAction(actionName);
        if(!act) { cout << "Unknown action\n"; return; }

        cout << "Enter notification lead times (e.g. 60m,30m or 1h). Blank for 10m: ";
        string notifyStr; getline(cin, notifyStr);
        std::vector<std::chrono::system_clock::duration> leadTimes;
        if(notifyStr.empty()) {
            leadTimes.push_back(std::chrono::minutes(10));
        } else {
            std::istringstream ns(notifyStr); string tok;
            auto parseDur = [](std::string t){
                t.erase(std::remove_if(t.begin(), t.end(), ::isspace), t.end());
                if(t.empty()) return std::chrono::minutes(0);
                char suf = t.back();
                int val;
                if(suf=='h' || suf=='H') { val = std::stoi(t.substr(0,t.size()-1)); return std::chrono::minutes(val*60); }
                if(suf=='m' || suf=='M') { val = std::stoi(t.substr(0,t.size()-1)); return std::chrono::minutes(val); }
                val = std::stoi(t); return std::chrono::minutes(val);
            };
            while(getline(ns,tok,',')) {
                if(tok.empty()) continue;
                leadTimes.push_back(parseDur(tok));
            }
            if(leadTimes.empty()) leadTimes.push_back(std::chrono::minutes(10));
        }

        string id = model_.generateUniqueId();
        auto notifyCb = [note,id,title](){ note(id,title); };
        OneTimeEvent e{id, desc, title, tp, std::chrono::seconds(0), "task"};
        // Stash names so ScheduledTask clone persists them to DB
        e.setNotifierName(notifierName);
        e.setActionName(actionName);
        scheduleTask(e, leadTimes, notifyCb, act);
        cout << "Added task [" << id << "]\n";
    };

    commands["remove"] = [&]() {
        cout << "Enter event ID to remove: ";
        string id; getline(cin, id);
        if (model_.removeEvent(id))
            cout << "Removed event [" << id << "]\n";
        else
            cout << "No event with ID [" << id << "] found.\n";
    };

    commands["removeday"] = [&]() {
        cout << "Enter date (YYYY-MM-DD): ";
        string d; getline(cin, d);
        system_clock::time_point day;
        try { day = TimeUtils::parseDate(d); } catch(const exception &e) { cout << e.what() << "\n"; return; }
        int n = model_.removeEventsOnDay(day);
        cout << "Removed " << n << " events.\n";
    };

    commands["removeweek"] = [&]() {
        cout << "Enter date within week (YYYY-MM-DD): ";
        string d; getline(cin, d);
        system_clock::time_point day;
        try { day = TimeUtils::parseDate(d); } catch(const exception &e) { cout << e.what() << "\n"; return; }
        int n = model_.removeEventsInWeek(day);
        cout << "Removed " << n << " events.\n";
    };

    commands["removebefore"] = [&]() {
        cout << "Enter time (YYYY-MM-DD HH:MM): ";
        string ts; getline(cin, ts);
        system_clock::time_point tp;
        try { tp = TimeUtils::parseTimePoint(ts); } catch(const exception &e) { cout << e.what() << "\n"; return; }
        int n = model_.removeEventsBefore(tp);
        cout << "Removed " << n << " events.\n";
    };

    commands["clear"] = [&]() {
        removeAllEvents();
        cout << "All events removed.\n";
    };

    commands["list"] = [&]() { view_.render(); };
    commands["next"] = [&]() { printNextEvent(); };

    commands["day"] = [&]() {
        cout << "Enter date (YYYY-MM-DD): ";
        string d; getline(cin, d);
        system_clock::time_point day;
        try { day = TimeUtils::parseDate(d); } catch(const exception &e) { cout << e.what() << "\n"; return; }
        auto evs = model_.getEventsOnDay(day);
        view_.renderEvents(evs);
    };

    commands["week"] = [&]() {
        cout << "Enter date within week (YYYY-MM-DD): ";
        string d; getline(cin, d);
        system_clock::time_point day;
        try { day = TimeUtils::parseDate(d); } catch(const exception &e) { cout << e.what() << "\n"; return; }
        auto evs = model_.getEventsInWeek(day);
        view_.renderEvents(evs);
    };

    commands["month"] = [&]() {
        cout << "Enter month (YYYY-MM): ";
        string m; getline(cin, m);
        system_clock::time_point mo;
        try { mo = TimeUtils::parseMonth(m); } catch(const exception &e) { cout << e.what() << "\n"; return; }
        auto evs = model_.getEventsInMonth(mo);
        view_.renderEvents(evs);
    };

    commands["nextn"] = [&]() {
        cout << "Enter number of events: ";
        int n; cin >> n; cin.ignore(numeric_limits<streamsize>::max(), '\n');
        auto evs = model_.getNextNEvents(n);
        view_.renderEvents(evs);
    };

    bool done = false;
    string line;
    cout << "Commands: add addat addrec addtask remove removeday removeweek removebefore clear list next day week month nextn quit\n";
    while (!done)
    {
        cout << "> ";
        if (!getline(cin, line)) break;
        istringstream iss(line); string cmd; iss >> cmd;
        auto it = commands.find(cmd);
        if (it != commands.end())
        {
            it->second();
            if (cmd == "quit") done = true; // handled below
        }
        else if (cmd == "quit")
        {
            done = true;
        }
        else
        {
            cout << "Unknown command.\n";
        }
    }

    cout << "Exiting scheduler.\n";
}

void Controller::scheduleTask(const Event &e,
                              std::vector<std::chrono::system_clock::duration> notifyBefore,
                              std::function<void()> notifyCb,
                              std::function<void()> actionCb)
{
    if (!loop_) return;
    if (!notifyCb)
        notifyCb = [id = e.getId(), title = e.getTitle()]() {
            std::cout << "[" << id << "] \"" << title << "\" notification\n";
        };
    if (!actionCb)
        actionCb = [id = e.getId(), title = e.getTitle()]() {
            std::cout << "[" << id << "] \"" << title << "\" executing\n";
        };
    auto now = std::chrono::system_clock::now();
    std::vector<std::chrono::system_clock::time_point> notifyTimes;
    if (e.getTime() - now >= std::chrono::minutes(10)) {
        for (auto d : notifyBefore) {
            auto tp = e.getTime() - d;
            if (tp > now)
                notifyTimes.push_back(tp);
        }
    }

    // Use a distinct ID for the scheduled task to avoid collisions with the
    // base calendar event stored in the model/database.
    std::string taskId = model_.generateUniqueId();
    auto task = std::make_shared<ScheduledTask>(
        taskId, e.getDescription(), e.getTitle(), e.getTime(), e.getDuration(),
        notifyTimes, std::move(notifyCb), std::move(actionCb));
    task->setCategory("task");
    task->setNotifierName(e.getNotifierName());
    task->setActionName(e.getActionName());
    loop_->addTask(task);
}

void Controller::removeAllEvents()
{
    model_.removeAllEvents();
}

void Controller::removeEventsOnDay(std::chrono::system_clock::time_point day)
{
    model_.removeEventsOnDay(day);
}

void Controller::removeEventsInWeek(std::chrono::system_clock::time_point day)
{
    model_.removeEventsInWeek(day);
}

void Controller::removeEventsBefore(std::chrono::system_clock::time_point time)
{
    model_.removeEventsBefore(time);
}
