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
#include "../utils/TimeUtils.h"
#include <stdexcept>
#include "../scheduler/ScheduledTask.h"
#include <memory>

using namespace std;
using namespace std::chrono;

Controller::Controller(Model &model, View &view, EventLoop *loop)
    : model_(model), view_(view), loop_(loop)
{
}

// Convert a UTC time_point to a local timestamp string
string Controller::formatTimePoint(const system_clock::time_point &tp)
{
    return TimeUtils::formatTimePoint(tp);
}

// Interpret a local timestamp string and convert it to a UTC time_point
system_clock::time_point Controller::parseTimePoint(const string &timestamp)
{
    return TimeUtils::parseTimePoint(timestamp);
}

system_clock::time_point Controller::parseDate(const string &dateStr)
{
    return TimeUtils::parseDate(dateStr);
}

system_clock::time_point Controller::parseMonth(const string &monthStr)
{
    return TimeUtils::parseMonth(monthStr);
}

void Controller::printNextEvent()
{
    try
    {
        Event next = model_.getNextEvent();
        cout << "Next event: ["
             << next.getId() << "] \""
             << next.getTitle() << "\" @ "
             << formatTimePoint(next.getTime())
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
    scheduleTask(e);
    return idCopy;
}

void Controller::run()
{
    cout << "=== Scheduler CLI ===\n";
    cout << "(All times are entered and displayed in local time,\n"
            " but stored internally in UTC.)\n";

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
        try { tp = parseTimePoint(timestr); }
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
        try { start = parseTimePoint(timestr); }
        catch(const exception &e) { cout << e.what() << "\n"; return; }
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
        string id = addRecurringEvent(title, desc, start, hours(1), pat);
        cout << "Added recurring event [" << id << "]\n";
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
        try { day = parseDate(d); } catch(const exception &e) { cout << e.what() << "\n"; return; }
        int n = model_.removeEventsOnDay(day);
        cout << "Removed " << n << " events.\n";
    };

    commands["removeweek"] = [&]() {
        cout << "Enter date within week (YYYY-MM-DD): ";
        string d; getline(cin, d);
        system_clock::time_point day;
        try { day = parseDate(d); } catch(const exception &e) { cout << e.what() << "\n"; return; }
        int n = model_.removeEventsInWeek(day);
        cout << "Removed " << n << " events.\n";
    };

    commands["removebefore"] = [&]() {
        cout << "Enter time (YYYY-MM-DD HH:MM): ";
        string ts; getline(cin, ts);
        system_clock::time_point tp;
        try { tp = parseTimePoint(ts); } catch(const exception &e) { cout << e.what() << "\n"; return; }
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
        try { day = parseDate(d); } catch(const exception &e) { cout << e.what() << "\n"; return; }
        auto evs = model_.getEventsOnDay(day);
        view_.renderEvents(evs);
    };

    commands["week"] = [&]() {
        cout << "Enter date within week (YYYY-MM-DD): ";
        string d; getline(cin, d);
        system_clock::time_point day;
        try { day = parseDate(d); } catch(const exception &e) { cout << e.what() << "\n"; return; }
        auto evs = model_.getEventsInWeek(day);
        view_.renderEvents(evs);
    };

    commands["month"] = [&]() {
        cout << "Enter month (YYYY-MM): ";
        string m; getline(cin, m);
        system_clock::time_point mo;
        try { mo = parseMonth(m); } catch(const exception &e) { cout << e.what() << "\n"; return; }
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
    cout << "Commands: add addat addrec remove removeday removeweek removebefore clear list next day week month nextn quit\n";
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
                              std::chrono::system_clock::duration notifyBefore,
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

    auto task = std::make_shared<ScheduledTask>(
        e.getId(), e.getDescription(), e.getTitle(), e.getTime(), e.getDuration(),
        notifyBefore, std::move(notifyCb), std::move(actionCb));
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
