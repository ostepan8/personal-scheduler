// Controller.cpp
#include "Controller.h"
#include "../model/OneTimeEvent.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

using namespace std;
using namespace std::chrono;

Controller::Controller(Model &model, View &view)
    : model_(model), view_(view)
{
}

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

void Controller::run()
{
    cout << "=== Scheduler CLI ===\n";
    cout << "Commands: add  remove  list  next  quit\n";

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
            // Prompt for ID, title, description, hours-from-now:
            cout << "Enter event ID: ";
            string id;
            getline(cin, id);
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
            cout << "Unknown command. Available: add  remove  list  next  quit\n";
        }
    }

    cout << "Exiting scheduler.\n";
}
