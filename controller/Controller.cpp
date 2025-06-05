// Controller.cpp
#include "Controller.h"
#include "../model/OneTimeEvent.h"
#include "../model/RecurringEvent.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
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
    time_t t_c = system_clock::to_time_t(tp);
    tm tm_buf;
#if defined(_MSC_VER)
    localtime_s(&tm_buf, &t_c);
#else
    localtime_r(&t_c, &tm_buf);
#endif
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm_buf);
    return string(buf);
}

// Interpret a local timestamp string and convert it to a UTC time_point
system_clock::time_point Controller::parseTimePoint(const string &timestamp)
{
    std::tm tm_buf{};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm_buf, "%Y-%m-%d %H:%M");
    if (ss.fail())
    {
        throw std::runtime_error("Invalid timestamp format");
    }
    // Let mktime determine daylight saving time so we don't misinterpret
    // timestamps in locales that observe DST. Without this, events entered
    // during DST could be shifted by one hour.
    tm_buf.tm_isdst = -1;
    time_t time_c = std::mktime(&tm_buf);
    return system_clock::from_time_t(time_c);
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
                                     RecurrencePattern &pattern)
{
    string idCopy = model_.generateUniqueId();
    string descCopy = desc;
    string titleCopy = title;
    RecurringEvent e(idCopy, descCopy, titleCopy, start, dur, pattern);
    model_.addEvent(e);
    return idCopy;
}

void Controller::run()
{
    cout << "=== Scheduler CLI ===\n";
    cout << "(All times are entered and displayed in local time,\n"
            " but stored internally in UTC.)\n";
    cout << "Commands: add  addat  remove  list  next  quit\n";

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
            cout << "Unknown command. Available: add  addat  remove  list  next  quit\n";
        }
    }

    cout << "Exiting scheduler.\n";
}
