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
#include "../utils/TimeUtils.h"
#include <stdexcept>

using namespace std;
using namespace std::chrono;

Controller::Controller(Model &model, View &view)
    : model_(model), view_(view)
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
    return idCopy;
}

void Controller::run()
{
    cout << "=== Scheduler CLI ===\n";
    cout << "(All times are entered and displayed in local time,\n"
            " but stored internally in UTC.)\n";
    cout << "Commands: add  addat  addrec  remove  list  next  quit\n";

    string line;
    while (true)
    {
        cout << "> ";
        if (!getline(cin, line))
        {
            break;
        }
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        if (cmd == "add")
        {
            // Prompt for title, description, hours-from-now:
            string id = model_.generateUniqueId();
            cout << "Enter title: ";
            string title;
            getline(cin, title);
            cout << "Enter description: ";
            string desc;
            getline(cin, desc);
            cout << "Enter hours from now (integer): ";
            int hrs;
            cin >> hrs;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            auto now = system_clock::now();
            system_clock::time_point tp = now + hours(hrs);
            system_clock::duration dur = hours(1);

            OneTimeEvent e{id, desc, title, tp, dur};
            model_.addEvent(e);
            cout << "Added event [" << id << "]\n";
        }
        else if (cmd == "addat")
        {
            // Prompt for title, description, timestamp
            string id = model_.generateUniqueId();
            cout << "Enter title: ";
            string title;
            getline(cin, title);
            cout << "Enter description: ";
            string desc;
            getline(cin, desc);
            cout << "Enter time (YYYY-MM-DD HH:MM): ";
            string timestr;
            getline(cin, timestr);
            system_clock::time_point tp;
            try
            {
                tp = parseTimePoint(timestr);
            }
            catch (const std::exception &e)
            {
                cout << e.what() << "\n";
                continue;
            }
            system_clock::duration dur = hours(1);

            OneTimeEvent e{id, desc, title, tp, dur};
            model_.addEvent(e);
            cout << "Added event [" << id << "]\n";
        }
        else if (cmd == "addrec")
        {
            string title, desc, timestr, rtype;
            cout << "Enter title: ";
            getline(cin, title);
            cout << "Enter description: ";
            getline(cin, desc);
            cout << "Enter start time (YYYY-MM-DD HH:MM): ";
            getline(cin, timestr);
            system_clock::time_point start;
            try
            {
                start = parseTimePoint(timestr);
            }
            catch (const std::exception &e)
            {
                cout << e.what() << "\n";
                continue;
            }
            cout << "Recurrence type (daily/weekly): ";
            getline(cin, rtype);
            std::shared_ptr<RecurrencePattern> pat;
            if (rtype == "weekly" || rtype == "w")
            {
                cout << "Interval in weeks: ";
                int weeks;
                cin >> weeks;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Days of week (0=Sun..6=Sat comma separated): ";
                string daysStr;
                getline(cin, daysStr);
                vector<Weekday> days;
                string tmp;
                istringstream ds(daysStr);
                while (getline(ds, tmp, ','))
                {
                    int d = stoi(tmp);
                    days.push_back(static_cast<Weekday>(d));
                }
                pat = make_shared<WeeklyRecurrence>(start, days, weeks);
            }
            else
            {
                cout << "Interval in days: ";
                int days;
                cin >> days;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                pat = make_shared<DailyRecurrence>(start, days);
            }

            system_clock::duration dur = hours(1);
            string id = addRecurringEvent(title, desc, start, dur, pat);
            cout << "Added recurring event [" << id << "]\n";
        }
        else if (cmd == "remove")
        {
            cout << "Enter event ID to remove: ";
            string id;
            getline(cin, id);
            if (model_.removeEvent(id))
            {
                cout << "Removed event [" << id << "]\n";
            }
            else
            {
                cout << "No event with ID [" << id << "] found.\n";
            }
        }
        else if (cmd == "list")
        {
            view_.render();
        }
        else if (cmd == "next")
        {
            printNextEvent();
        }
        else if (cmd == "quit")
        {
            break;
        }
        else
        {
            cout << "Unknown command. Available: add  addat  addrec  remove  list  next  quit\n";
        }
    }

    cout << "Exiting scheduler.\n";
}
