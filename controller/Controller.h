// Controller.h
#pragma once

#include <string>
#include "../model/Model.h"
#include "../model/recurrence/RecurrencePattern.h"
#include "../view/View.h"
#include "../scheduler/EventLoop.h"
#include <chrono>
#include <functional>

/*
  Controller coordinates a Model and a View.  It runs a simple CLI loop:
    - "add" prompts for title, description, and hours-from-now → adds OneTimeEvent with an auto-generated ID.
    - "addat" prompts for title, description, and timestamp → adds OneTimeEvent at a specific time with an auto-generated ID.
    - "addrec" prompts for recurrence details → adds a RecurringEvent.
    - "remove" prompts for ID → removes that event.
    - "list" → asks View to render current Model state.
    - "next" → prints the next upcoming event.
    - "quit" → exits.
*/
class Controller
{
public:
    // Constructor takes a reference to a Model (to mutate) and a View (to render).
    Controller(Model &model, View &view, EventLoop *loop = nullptr);

    // Run the main command loop until “quit” is entered.
    void run();

    // Remove all events from the model
    void removeAllEvents();

    // Remove events on a specific day
    void removeEventsOnDay(std::chrono::system_clock::time_point day);

    // Remove events in the week containing the given day
    void removeEventsInWeek(std::chrono::system_clock::time_point day);

    // Remove events before the given time
    void removeEventsBefore(std::chrono::system_clock::time_point time);

private:
    Model &model_;
    View &view_;
    EventLoop *loop_;

    // Convert a UTC time_point to a local time string "YYYY-MM-DD HH:MM".
    static std::string formatTimePoint(const std::chrono::system_clock::time_point &tp);

    // Parse a local time string "YYYY-MM-DD HH:MM" and return a UTC time_point.
    static std::chrono::system_clock::time_point
    parseTimePoint(const std::string &timestamp);

    static std::chrono::system_clock::time_point
    parseDate(const std::string &dateStr);

    static std::chrono::system_clock::time_point
    parseMonth(const std::string &monthStr);

    // Print the next upcoming event or “no upcoming events”.
    void printNextEvent();

    void scheduleTask(const Event &e,
                      std::vector<std::chrono::system_clock::duration> notifyBefore =
                          {std::chrono::minutes(10)},
                      std::function<void()> notifyCb = {},
                      std::function<void()> actionCb = {});

    // Add a recurring event using an existing RecurrencePattern. Returns the ID of the new event.
    std::string addRecurringEvent(const std::string &title,
                                  const std::string &desc,
                                  std::chrono::system_clock::time_point start,
                                  std::chrono::system_clock::duration dur,
                                  std::shared_ptr<RecurrencePattern> pattern);
};
